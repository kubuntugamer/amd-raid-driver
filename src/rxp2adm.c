// SPDX-License-Identifier: GPL-2.0-only
/*
 * rxp2adm — clean-room array management CLI for the rcraid driver
 *
 * Talks only to /dev/amd_ctl0 (AMD_RAID_MAGIC 'a').  It does not patch,
 * wrap, or otherwise touch AMD's rcadm binary; the licence check in that
 * tool does not exist here because this is a different program.
 *
 * Levels: the driver's metadata writer emits RAID 0/1/10, and RAID 5 when
 * loaded with parity_create=1 (metadata only, over erased members).  RAID 6
 * is refused by the driver with its reason rather than a usage error here.
 * A parity array written elsewhere is assembled and read; writes need the
 * parity_writes=1 opt-in.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "amd_raid_uapi.h"

#define CTL_DEV "/dev/amd_ctl0"

static void usage(FILE *out)
{
	fprintf(out,
		"usage: rxp2adm <verb> [options]\n"
		"\n"
		"verbs:\n"
		"  list                         list disks and arrays (-M -qa)\n"
		"  create -l LEVEL -d D... [-s MiB]\n"
		"                               stamp an array over disk indices\n"
		"  delete [-a ID]               delete array ID (default 0)\n"
		"\n"
		"levels: 0, 1, 5, 10 (alias raid0/raid1/raid5/raid10)\n"
		"        5 needs the module loaded with parity_create=1 and members\n"
		"        whose user area is empty; CREATE writes metadata only and\n"
		"        does not zero data or parity.  6 is refused.\n"
		"\n"
		"examples:\n"
		"  rxp2adm list\n"
		"  rxp2adm create -l raid10 -d 0 1 2 3\n"
		"  rxp2adm -C -r1 -d 0 1\n"
		"  rxp2adm delete -a 0\n");
}

static const char *level_name(uint32_t level)
{
	switch (level) {
	case AMD_ARRAY_LEVEL_RAID0:  return "RAID0";
	case AMD_ARRAY_LEVEL_RAID1:  return "RAID1";
	case AMD_ARRAY_LEVEL_RAID5:  return "RAID5";
	case AMD_ARRAY_LEVEL_RAID6:  return "RAID6";
	case AMD_ARRAY_LEVEL_RAID10: return "RAID10";
	default:                     return "?";
	}
}

static int parse_level(const char *s, uint32_t *out)
{
	static const struct {
		const char *name;
		uint32_t level;
	} map[] = {
		{ "0", AMD_ARRAY_LEVEL_RAID0 },
		{ "1", AMD_ARRAY_LEVEL_RAID1 },
		{ "5", AMD_ARRAY_LEVEL_RAID5 },
		{ "6", AMD_ARRAY_LEVEL_RAID6 },
		{ "10", AMD_ARRAY_LEVEL_RAID10 },
		{ "raid0", AMD_ARRAY_LEVEL_RAID0 },
		{ "raid1", AMD_ARRAY_LEVEL_RAID1 },
		{ "raid5", AMD_ARRAY_LEVEL_RAID5 },
		{ "raid6", AMD_ARRAY_LEVEL_RAID6 },
		{ "raid10", AMD_ARRAY_LEVEL_RAID10 },
	};
	size_t i;

	for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		if (!strcmp(s, map[i].name)) {
			*out = map[i].level;
			return 0;
		}
	}
	return -1;
}

static int open_ctl(int flags)
{
	int fd = open(CTL_DEV, flags);

	if (fd < 0)
		fprintf(stderr, "rxp2adm: cannot open %s: %s (is rcraid loaded?)\n",
			CTL_DEV, strerror(errno));
	return fd;
}

static void read_attr(const char *disk, const char *attr, char *buf, size_t len)
{
	char path[160];
	FILE *f;

	snprintf(path, sizeof(path), "/sys/block/%s/%s", disk, attr);
	f = fopen(path, "r");
	if (!f) {
		snprintf(buf, len, "?");
		return;
	}
	if (!fgets(buf, (int)len, f))
		buf[0] = '\0';
	fclose(f);
	buf[strcspn(buf, "\n")] = '\0';
}

static int cmd_list(void)
{
	struct AmdHardwarePayload *p;
	int fd, i, ret = 0;

	p = calloc(1, sizeof(*p));
	if (!p) {
		perror("calloc");
		return 1;
	}
	fd = open_ctl(O_RDONLY);
	if (fd < 0) {
		free(p);
		return 2;
	}
	if (ioctl(fd, AMD_IOCTL_GET_HARDWARE_INVENTORY, p) < 0) {
		fprintf(stderr, "rxp2adm: inventory ioctl failed: %s\n",
			strerror(errno));
		close(fd);
		free(p);
		return 1;
	}

	printf("Disks (rcbottom members):\n");
	if (!p->total_discovered_drives)
		printf("  none\n");
	for (i = 0; i < (int)p->total_discovered_drives; i++) {
		const struct AmdDriveTelemetry *d = &p->drives[i];
		double tb = (double)d->total_sectors * 512.0 / 1e12;

		printf("  %2u  %-6s %7.2f TB\n", d->disk_index,
		       d->is_assigned ? "member" : "free", tb);
	}

	printf("Arrays:\n");
	if (!p->total_discovered_arrays)
		printf("  none\n");
	for (i = 0; i < (int)p->total_discovered_arrays; i++) {
		const struct AmdArrayTelemetry *a = &p->arrays[i];
		char state[32], ro[8];
		double tb = (double)a->total_capacity_bytes / 1e12;

		read_attr(a->name, "rcraid/state", state, sizeof(state));
		read_attr(a->name, "ro", ro, sizeof(ro));
		printf("  %2u  %-6s %-10s %7.2f TB  ro=%s  members=%u  /dev/%s\n",
		       a->array_id, level_name(a->raid_level), state, tb, ro,
		       a->member_disk_count, a->name);
	}

	close(fd);
	free(p);
	return ret;
}

static int cmd_create(uint32_t level, const uint32_t *disks, uint32_t n,
		      uint32_t size_mb)
{
	struct amd_array_create req;
	int fd, err;

	if (n < 2 || n > AMD_ARRAY_MAX_MEMBERS) {
		fprintf(stderr, "rxp2adm: need 2..%u disks, got %u\n",
			AMD_ARRAY_MAX_MEMBERS, n);
		return 2;
	}
	memset(&req, 0, sizeof(req));
	req.level = level;
	req.member_count = n;
	req.size_mb = size_mb;
	memcpy(req.member_indices, disks, n * sizeof(disks[0]));

	fd = open_ctl(O_RDWR);
	if (fd < 0)
		return 2;
	if (ioctl(fd, AMD_IOCTL_CREATE_ARRAY, &req) < 0) {
		err = errno;
		close(fd);
		if ((level == AMD_ARRAY_LEVEL_RAID5 ||
		     level == AMD_ARRAY_LEVEL_RAID6) && err == EOPNOTSUPP) {
			fprintf(stderr,
				"rxp2adm: RAID %u create is disabled:\n"
				"  5 needs the module loaded with parity_create=1 and\n"
				"  members whose user area is empty (metadata only; the\n"
				"  data/parity region is not zeroed).\n"
				"  6 has no Q write/initialisation path.\n",
				level);
		} else if (err == EPERM || err == EACCES) {
			fprintf(stderr, "rxp2adm: create requires root (CAP_SYS_ADMIN)\n");
		} else {
			fprintf(stderr, "rxp2adm: create failed: %s\n",
				strerror(err));
		}
		return 1;
	}
	close(fd);
	printf("rxp2adm: RAID%u created over %u disk(s)\n", level, n);
	if (level == AMD_ARRAY_LEVEL_RAID5)
		printf("rxp2adm: warning: RAID5 metadata only — data/parity are not initialised; zero the volume before use\n");
	return 0;
}

static int cmd_delete(uint32_t array_id)
{
	struct amd_array_delete req;
	int fd;

	memset(&req, 0, sizeof(req));
	req.array_id = array_id;
	fd = open_ctl(O_RDWR);
	if (fd < 0)
		return 2;
	if (ioctl(fd, AMD_IOCTL_DELETE_ARRAY, &req) < 0) {
		int err = errno;

		close(fd);
		if (err == ENODEV)
			fprintf(stderr, "rxp2adm: no array is assembled\n");
		else if (err == EPERM || err == EACCES)
			fprintf(stderr, "rxp2adm: delete requires root (CAP_SYS_ADMIN)\n");
		else
			fprintf(stderr, "rxp2adm: delete failed: %s\n",
				strerror(err));
		return 1;
	}
	close(fd);
	printf("rxp2adm: array %u deleted\n", array_id);
	return 0;
}

int main(int argc, char **argv)
{
	uint32_t disks[AMD_ARRAY_MAX_MEMBERS];
	uint32_t level = 0xffffffffu, ndisks = 0, size_mb = 0, array_id = 0;
	int do_list = 0, do_create = 0, do_delete = 0;
	int i;

	if (argc < 2) {
		usage(stderr);
		return 2;
	}

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (!strcmp(a, "list") || !strcmp(a, "-M")) {
			do_list = 1;
		} else if (!strcmp(a, "create") || !strcmp(a, "-C")) {
			do_create = 1;
		} else if (!strcmp(a, "delete") || !strcmp(a, "-D")) {
			do_delete = 1;
		} else if (!strcmp(a, "-qa") || !strcmp(a, "-na") ||
			   !strcmp(a, "--quiet")) {
			/* accepted for muscle memory; nothing to suppress */
		} else if (!strncmp(a, "-r", 2) && a[2]) {
			if (parse_level(a + 2, &level) < 0) {
				fprintf(stderr, "rxp2adm: unknown level '%s'\n", a + 2);
				return 2;
			}
		} else if (!strcmp(a, "-l") || !strcmp(a, "--level")) {
			if (++i >= argc || parse_level(argv[i], &level) < 0) {
				fprintf(stderr, "rxp2adm: -l needs a level\n");
				return 2;
			}
		} else if (!strcmp(a, "-d") || !strcmp(a, "--disks")) {
			while (i + 1 < argc && argv[i + 1][0] != '-') {
				char *end;
				unsigned long v;

				errno = 0;
				v = strtoul(argv[++i], &end, 10);
				if (errno || *end || v >= AMD_ARRAY_MAX_DISKS ||
				    ndisks >= AMD_ARRAY_MAX_MEMBERS) {
					fprintf(stderr, "rxp2adm: bad disk index '%s'\n",
						argv[i]);
					return 2;
				}
				disks[ndisks++] = (uint32_t)v;
			}
			if (!ndisks) {
				fprintf(stderr, "rxp2adm: -d needs disk indices\n");
				return 2;
			}
		} else if (!strcmp(a, "-s") || !strcmp(a, "--size")) {
			char *end;
			unsigned long v;

			if (++i >= argc) {
				fprintf(stderr, "rxp2adm: -s needs MiB\n");
				return 2;
			}
			errno = 0;
			v = strtoul(argv[i], &end, 10);
			if (errno || *end)
				return 2;
			size_mb = (uint32_t)v;
		} else if (!strcmp(a, "-a") || !strcmp(a, "--array")) {
			char *end;
			unsigned long v;

			if (++i >= argc) {
				fprintf(stderr, "rxp2adm: -a needs an array id\n");
				return 2;
			}
			errno = 0;
			v = strtoul(argv[i], &end, 10);
			if (errno || *end)
				return 2;
			array_id = (uint32_t)v;
		} else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
			usage(stdout);
			return 0;
		} else {
			fprintf(stderr, "rxp2adm: unknown argument '%s'\n", a);
			usage(stderr);
			return 2;
		}
	}

	if (do_list + do_create + do_delete != 1) {
		fprintf(stderr, "rxp2adm: exactly one verb is required\n");
		usage(stderr);
		return 2;
	}
	if (do_create) {
		if (level == 0xffffffffu) {
			fprintf(stderr, "rxp2adm: create needs -l LEVEL\n");
			return 2;
		}
		if (!ndisks) {
			fprintf(stderr, "rxp2adm: create needs -d DISK...\n");
			return 2;
		}
		return cmd_create(level, disks, ndisks, size_mb);
	}
	if (do_delete)
		return cmd_delete(array_id);

	return cmd_list();
}
