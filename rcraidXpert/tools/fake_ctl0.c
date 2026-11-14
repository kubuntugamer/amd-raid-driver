/*
 * fake_ctl0 - LD_PRELOAD shim that stands in for /dev/amd_ctl0.
 *
 * This is a development aid: it lets rcraidXpert be exercised without
 * loading rcraid.ko or binding any PCI device.  It is never linked into the
 * GUI and never shipped; run it through tools/run-fake.sh.
 *
 * Environment:
 *   FAKE_CTL_ABSENT=1        make open() fail, as if the module were missing
 *   FAKE_CTL_DISKS=N         number of drives in the inventory (default 4)
 *   FAKE_CTL_LEVEL=L         RAID level reported (0/1/5/10, default 5)
 *   FAKE_CTL_MEMBERS=N       members taking part in the array (default 3)
 *   FAKE_CTL_DEGRADED=1      report the volume as degraded
 *   FAKE_CTL_CREATE_ERRNO=E  make CREATE fail with errno E (e.g. 95)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "amd_raid_uapi.h"

#define FAKE_FD 4242

static int env_int(const char *name, int fallback)
{
	const char *value = getenv(name);

	if (!value || !*value)
		return fallback;
	return atoi(value);
}

static int fake_open(const char *path)
{
	if (strcmp(path, "/dev/amd_ctl0") != 0)
		return -1;
	if (env_int("FAKE_CTL_ABSENT", 0)) {
		errno = ENOENT;
		return -1;
	}
	return FAKE_FD;
}

int open(const char *path, int flags, ...)
{
	static int (*real_open)(const char *, int, ...);
	mode_t mode = 0;
	int fake;
	va_list ap;

	fake = fake_open(path);
	if (fake >= 0)
		return fake;

	if (!real_open)
		real_open = dlsym(RTLD_NEXT, "open");
	if (flags & O_CREAT) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}
	return real_open(path, flags, mode);
}

int open64(const char *path, int flags, ...)
{
	static int (*real_open64)(const char *, int, ...);
	mode_t mode = 0;
	int fake;
	va_list ap;

	fake = fake_open(path);
	if (fake >= 0)
		return fake;

	if (!real_open64)
		real_open64 = dlsym(RTLD_NEXT, "open64");
	if (flags & O_CREAT) {
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}
	return real_open64(path, flags, mode);
}

static void fill_inventory(struct AmdHardwarePayload *payload)
{
	int ndisks = env_int("FAKE_CTL_DISKS", 4);
	int nmembers = env_int("FAKE_CTL_MEMBERS", 3);
	int level = env_int("FAKE_CTL_LEVEL", 5);
	int i;

	if (ndisks < 0)
		ndisks = 0;
	if (ndisks > 64)
		ndisks = 64;
	if (nmembers > ndisks)
		nmembers = ndisks;

	memset(payload, 0, sizeof(*payload));
	payload->total_discovered_drives = ndisks;
	for (i = 0; i < ndisks; i++) {
		payload->drives[i].disk_index = i;
		payload->drives[i].total_sectors = 7814037168ull;
		payload->drives[i].is_assigned = i < nmembers;
	}

	if (nmembers < 2)
		return;

	payload->total_discovered_arrays = 1;
	payload->arrays[0].array_id = 0;
	payload->arrays[0].raid_level = level;
	payload->arrays[0].stripe_sectors = 1536;
	payload->arrays[0].total_capacity_bytes = 256ull * 1024 * 1024;
	payload->arrays[0].member_disk_count = nmembers;
	for (i = 0; i < nmembers && i < 32; i++)
		payload->arrays[0].member_indices[i] = i;
	strcpy(payload->arrays[0].name, "rcraid0");
}

static int fake_ioctl(unsigned long request, void *arg)
{
	if (request == AMD_IOCTL_GET_HARDWARE_INVENTORY) {
		fill_inventory(arg);
		return 0;
	}
	if (request == AMD_IOCTL_GET_STATUS) {
		unsigned int mask = RC_AMD_STATUS_LIVE;

		if (env_int("FAKE_CTL_DEGRADED", 0))
			mask |= RC_AMD_STATUS_DEGRADED;
		*(unsigned int *)arg = mask;
		return 0;
	}
	if (request == AMD_IOCTL_CREATE_ARRAY) {
		int fail = env_int("FAKE_CTL_CREATE_ERRNO", 0);

		if (fail) {
			errno = fail;
			return -1;
		}
		return 0;
	}
	if (request == AMD_IOCTL_DELETE_ARRAY)
		return 0;

	errno = EINVAL;
	return -1;
}

int ioctl(int fd, unsigned long request, ...)
{
	static int (*real_ioctl)(int, unsigned long, ...);
	va_list ap;
	void *arg;
	int ret;

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);

	if (fd == FAKE_FD)
		return fake_ioctl(request, arg);

	if (!real_ioctl)
		real_ioctl = dlsym(RTLD_NEXT, "ioctl");
	ret = real_ioctl(fd, request, arg);
	return ret;
}
