// SPDX-License-Identifier: GPL-2.0-only
/*
 * AMD-RAID Linux driver — /dev/rc_api SG_IO compatibility surface
 *
 * Copyright (C) 2025-2026 Joey Troy and contributors.
 *
 * Presents the config-packet ABI that AMD's rcadm CLI speaks: a
 * character device driven by SG_IO, an INQUIRY EVPD page 0xD7 identify
 * string, and a 64 KiB config packet read through a mailbox window.  The
 * packet is built from the volume this driver actually assembled (member
 * registry plus the RC_LogicalDevice record parsed off disk), so rcadm
 * sees the array the driver sees rather than a second copy of firmware
 * state.
 *
 * Read-only by construction: this file never writes metadata.  rcadm's
 * only observed write is the 8 KiB mailbox descriptor that precedes a
 * window read (command word 1); every other command is refused with
 * CHECK CONDITION instead of being silently accepted.
 *
 * Original work, independently authored from clean-room reverse
 * engineering of the AMD-RAID Windows driver binaries under DMCA
 * §1201(f) interoperability protections.  See docs/RE_METHODOLOGY.md and
 * the "rcadm /dev/rc_api config-packet ABI" section of
 * docs/REVERSE_ENGINEERING.md.
 */

#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <scsi/sg.h>

#include "rc_linux.h"
#include "rc_api.h"

#define RC_API_DEVICE_NAME	"rc_api"
#define RC_API_ID_STRING	"AMD-RAID Controller"

/* Config-packet header (packed, 512 B) field offsets. */
#define RC_API_HDR_VERSION	0x14
#define RC_API_HDR_PACKET_SIZE	0x18
#define RC_API_HDR_PD_OFFSET	0x24
#define RC_API_HDR_PD_SIZE	0x28
#define RC_API_HDR_LD_OFFSET	0x2C
#define RC_API_HDR_LD_SIZE	0x30
#define RC_API_HDR_GENERATION	0x34
#define RC_API_HDR_CTRL_OFFSET	0x40
#define RC_API_HDR_CTRL_SIZE	0x44
#define RC_API_HDR_SEP_OFFSET	0x48
#define RC_API_HDR_SEP_SIZE	0x4C

/* PhysicalDevice record (tag 0x25BC) field offsets in its 256-byte slot. */
#define RC_API_PD_TAG		0x00
#define RC_API_PD_DEVICE_ID	0x04
#define RC_API_PD_DEVICE_NUMBER	0x0C	/* 0x10000 | slot in the vendor's packets */
#define RC_API_PD_DEVICES	0x18
#define RC_API_PD_CAPACITY	0x1C
#define RC_API_PD_DEVICE_TYPE	0x24
#define RC_API_PD_STATE		0x2C
#define RC_API_PD_FIRMWARE	0x80
#define RC_API_PD_MODEL		0x88
#define RC_API_PD_SERIAL	0xB0

/* DeviceType / state values observed in the vendor's own packet. */
#define RC_API_PD_TYPE_DISK	0x1200	/* NVMe member */
#define RC_API_PD_TYPE_CONTROLLER 0x1201	/* controller/version record */
#define RC_API_PD_STATE_ONLINE	0x3000

/* The only mailbox command rcadm was observed to issue (read-config). */
#define RC_API_MAILBOX_CMD_CONFIG 1u

#define RC_API_OP_INQUIRY	0x12
#define RC_API_OP_READ_10	0x28
#define RC_API_OP_WRITE_10	0x2a

#define RC_API_ASC_INVALID_OPCODE 0x20
#define RC_API_ASC_INVALID_FIELD  0x24

/*
 * The 1 KiB read at LBA 0 is RC_GetChannelID()'s probe: its first u32 is
 * the channel (byte offset >> 9) where the config window lives.  The
 * vendor walks 0xC0 -> 0xD0 -> 0xE0 across successive queries.
 */
#define RC_API_WINDOW_FIRST	0xc0u
#define RC_API_WINDOW_STEP	0x10u
#define RC_API_WINDOW_LAST	0xe0u

struct rc_api_session {
	u32 next_window;
};

static bool rc_api_registered;

static void rc_api_put_le32(u8 *p, u32 v)
{
	p[0] = (u8)v;
	p[1] = (u8)(v >> 8);
	p[2] = (u8)(v >> 16);
	p[3] = (u8)(v >> 24);
}

static void rc_api_put_le64(u8 *p, u64 v)
{
	rc_api_put_le32(p, (u32)v);
	rc_api_put_le32(p + 4, (u32)(v >> 32));
}

/*
 * The vendor spells its version "9.3.3-00302" in the packet (dash before
 * the maintenance number), while RC_DRIVER_VERSION is "9.3.3.00291".
 * rcadm prints this string verbatim, so match the vendor's separators.
 */
static void rc_api_version_string(u8 *dst, u32 len)
{
	const char *ver = RC_DRIVER_VERSION;
	u32 i, dots = 0, last = 0;

	memset(dst, 0, len);
	for (i = 0; ver[i] && i < len; i++) {
		dst[i] = ver[i];
		if (ver[i] == '.') {
			dots++;
			last = i;
		}
	}
	if (dots == 3 && last < len)
		dst[last] = '-';
}

/*
 * Build the 64 KiB packet rcadm parses.  The walker starts at packet+0x200
 * and strides by tag, so the section order only has to be self-consistent;
 * the header offsets are emitted conventionally (as the vendor does).
 */
static void rc_api_build_packet(u8 *pkt, const struct rc_api_snapshot *s)
{
	u32 nr_pd = s->nr_members ? s->nr_members + 1 : 0;
	u32 pd_bytes = nr_pd * RC_API_PD_STRIDE;
	u32 ld_off = RC_API_PD_BASE + pd_bytes;
	u32 ld_len = s->have_ld ? min_t(u32, s->ld_len, RC_API_LD_MAX) : 0;
	u32 i;

	rc_api_put_le32(pkt + RC_API_HDR_VERSION, 1);
	rc_api_put_le32(pkt + RC_API_HDR_PACKET_SIZE, RC_API_PACKET_SIZE);
	rc_api_put_le64(pkt + RC_API_HDR_GENERATION, 1);
	rc_api_put_le32(pkt + RC_API_HDR_PD_OFFSET, RC_API_PD_BASE);
	rc_api_put_le32(pkt + RC_API_HDR_PD_SIZE, pd_bytes);
	rc_api_put_le32(pkt + RC_API_HDR_LD_OFFSET, ld_off);
	rc_api_put_le32(pkt + RC_API_HDR_LD_SIZE, ld_len);
	rc_api_put_le32(pkt + RC_API_HDR_CTRL_OFFSET, ld_off + ld_len);
	rc_api_put_le32(pkt + RC_API_HDR_CTRL_SIZE, 0);
	rc_api_put_le32(pkt + RC_API_HDR_SEP_OFFSET, ld_off + ld_len);
	rc_api_put_le32(pkt + RC_API_HDR_SEP_SIZE, 0);

	if (!nr_pd)
		return;

	/*
	 * First PD slot is the controller/version record: DeviceType 0x1201,
	 * zero capacity, and the version string in the model slot (+0x88) —
	 * that is where rcadm's <VERSIONS> block reads it from.
	 */
	{
		u8 *pd = pkt + RC_API_PD_BASE;

		rc_api_put_le32(pd + RC_API_PD_TAG, RC_DST_PHYSICAL_DEVICE);
		rc_api_put_le32(pd + RC_API_PD_DEVICE_TYPE,
				RC_API_PD_TYPE_CONTROLLER);
		rc_api_put_le32(pd + RC_API_PD_STATE, RC_API_PD_STATE_ONLINE);
		rc_api_put_le32(pd + RC_API_PD_DEVICE_NUMBER, 0xffffu);
		rc_api_version_string(pd + RC_API_PD_MODEL, 32);
	}

	for (i = 0; i < s->nr_members; i++) {
		const struct rc_api_member *m = &s->member[i];
		u8 *pd = pkt + RC_API_PD_BASE + (i + 1) * RC_API_PD_STRIDE;

		rc_api_put_le32(pd + RC_API_PD_TAG, RC_DST_PHYSICAL_DEVICE);
		rc_api_put_le64(pd + RC_API_PD_DEVICE_ID, m->device_id);
		rc_api_put_le32(pd + RC_API_PD_DEVICE_NUMBER, 0x10000u + i);
		rc_api_put_le32(pd + RC_API_PD_DEVICES, 0);
		rc_api_put_le64(pd + RC_API_PD_CAPACITY, m->capacity_sectors);
		rc_api_put_le32(pd + RC_API_PD_DEVICE_TYPE, RC_API_PD_TYPE_DISK);
		rc_api_put_le32(pd + RC_API_PD_STATE,
				m->online ? RC_API_PD_STATE_ONLINE : 0);
		memcpy(pd + RC_API_PD_FIRMWARE, m->firmware,
		       sizeof(m->firmware));
		memcpy(pd + RC_API_PD_MODEL, m->model, sizeof(m->model));
		memcpy(pd + RC_API_PD_SERIAL, m->serial, sizeof(m->serial));
	}

	/* Re-emit the record verbatim: Id and element table are the
	 * firmware's, so rcadm correlates members the same way we do. */
	if (ld_len)
		memcpy(pkt + ld_off, s->ld_raw, ld_len);
}

/* Fixed-format sense data + CHECK CONDITION, the SG_IO way of failing a
 * command without failing the ioctl itself.  Never silent: every refusal
 * also lands in dmesg. */
static long rc_api_reject(struct sg_io_hdr *hdr, u8 asc)
{
	u8 sb[18];
	u32 n = 0;

	memset(sb, 0, sizeof(sb));
	sb[0] = 0x70;		/* current error, fixed format */
	sb[2] = 0x05;		/* ILLEGAL REQUEST */
	sb[7] = 10;		/* additional sense length */
	sb[12] = asc;

	if (hdr->sbp && hdr->mx_sb_len) {
		n = min_t(u32, sizeof(sb), hdr->mx_sb_len);
		if (copy_to_user(hdr->sbp, sb, n))
			return -EFAULT;
	}
	hdr->sb_len_wr = (u8)n;
	hdr->status = 0x02;			/* CHECK CONDITION */
	hdr->masked_status = (0x02 << 1) | 1;
	hdr->resid = (int)hdr->dxfer_len;

	rc_printk(RC_WARN, "/dev/rc_api: SG_IO command refused (ASC 0x%02x)\n",
		  asc);
	return 0;
}

static long rc_api_inquiry(struct sg_io_hdr *hdr, const u8 *cdb)
{
	u8 buf[36];
	u32 n;

	rc_printk(RC_INFO, "/dev/rc_api: INQUIRY evpd=%u page=0x%02x len=%u\n",
		  (unsigned int)(cdb[1] & 1), (unsigned int)cdb[2], hdr->dxfer_len);

	if (!hdr->dxfer_len || !hdr->dxferp)
		return rc_api_reject(hdr, RC_API_ASC_INVALID_FIELD);

	if (cdb[1] & 0x01) {	/* EVPD */
		if (cdb[2] != 0xd7)
			return rc_api_reject(hdr, RC_API_ASC_INVALID_FIELD);

		/*
		 * The vendor answers page 0xD7 with the bare string, not a
		 * page-code/length header: 20 bytes, NUL included.
		 */
		n = min_t(u32, hdr->dxfer_len, sizeof(RC_API_ID_STRING));
		if (copy_to_user(hdr->dxferp, RC_API_ID_STRING, n))
			return -EFAULT;
		hdr->resid = (int)(hdr->dxfer_len - n);
		return 0;
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = 0x00;			/* direct-access device */
	buf[2] = 0x05;			/* SPC-3 */
	buf[3] = 0x02;			/* response data format */
	buf[4] = sizeof(buf) - 5;
	memcpy(buf + 8, "AMD     ", 8);
	memcpy(buf + 16, "RAID Controller ", 16);
	memcpy(buf + 32, "0001", 4);

	n = min_t(u32, hdr->dxfer_len, sizeof(buf));
	if (copy_to_user(hdr->dxferp, buf, n))
		return -EFAULT;
	hdr->resid = (int)(hdr->dxfer_len - n);
	return 0;
}

static long rc_api_read(struct file *file, struct sg_io_hdr *hdr,
			const u8 *cdb, const struct rc_api_snapshot *snap,
			u8 *pkt)
{
	u64 lba = ((u64)cdb[2] << 24) | ((u64)cdb[3] << 16) |
		  ((u64)cdb[4] << 8) | (u64)cdb[5];
	u32 blocks = ((u32)cdb[7] << 8) | cdb[8];
	u32 want = blocks * 512u;
	u32 n;

	if (!hdr->dxfer_len || !hdr->dxferp)
		return rc_api_reject(hdr, RC_API_ASC_INVALID_FIELD);
	if (want > hdr->dxfer_len)
		want = hdr->dxfer_len;

	if (lba == 0 && want <= RC_API_DESCRIPTOR_BYTES) {
		struct rc_api_session *sess = file ? file->private_data : NULL;
		u32 win = sess ? sess->next_window : RC_API_WINDOW_FIRST;

		/*
		 * RC_GetChannelID() reads this 1 KiB and uses the first
		 * u32 as the config-window channel.  The packet follows at
		 * offset 4.  Zeros here mean channel 0 and rcadm stops
		 * with "no controllers, arrays, or disks supplied" before
		 * it ever issues the window read.
		 */
		rc_api_build_packet(pkt, snap);
		n = min_t(u32, want, RC_API_DESCRIPTOR_BYTES);
		if (n >= 4)
			rc_api_put_le32(pkt, win);
		if (sess && win < RC_API_WINDOW_LAST)
			sess->next_window = win + RC_API_WINDOW_STEP;
	} else {
		rc_api_build_packet(pkt, snap);
		n = min_t(u32, want, RC_API_PACKET_BYTES);
		rc_printk(RC_INFO,
			  "/dev/rc_api: READ(10) lba 0x%llx %u blocks → config packet (%u bytes)\n",
			  (unsigned long long)lba, blocks, n);
	}

	if (n && copy_to_user(hdr->dxferp, pkt, n))
		return -EFAULT;
	hdr->resid = (int)(hdr->dxfer_len - n);
	return 0;
}

static long rc_api_write(struct sg_io_hdr *hdr, const u8 *cdb)
{
	u64 lba = ((u64)cdb[2] << 24) | ((u64)cdb[3] << 16) |
		  ((u64)cdb[4] << 8) | (u64)cdb[5];
	u32 blocks = ((u32)cdb[7] << 8) | cdb[8];
	u32 want = blocks * 512u;
	u32 command, window;

	if (!hdr->dxfer_len || !hdr->dxferp)
		return rc_api_reject(hdr, RC_API_ASC_INVALID_FIELD);
	if (want > hdr->dxfer_len)
		want = hdr->dxfer_len;
	if (want < 8)
		return rc_api_reject(hdr, RC_API_ASC_INVALID_FIELD);
	if (get_user(command, (u32 __user *)hdr->dxferp))
		return -EFAULT;
	if (get_user(window, (u32 __user *)hdr->dxferp + 1))
		return -EFAULT;

	/*
	 * Command word 1 is the read-config window preparation: rcadm
	 * writes it immediately before reading the same window, and the
	 * reply does not depend on it.  Anything else could only be a
	 * configuration change, which this driver does not implement (it
	 * never writes firmware metadata) — refuse loudly rather than
	 * report a change that never happened.
	 */
	if (command != RC_API_MAILBOX_CMD_CONFIG) {
		rc_printk(RC_WARN,
			  "/dev/rc_api: REFUSED mailbox command %u (lba 0x%llx, %u bytes) — this driver never writes firmware metadata\n",
			  command, (unsigned long long)lba, want);
		return rc_api_reject(hdr, RC_API_ASC_INVALID_OPCODE);
	}

	rc_printk(RC_NOTE,
		  "/dev/rc_api: mailbox prepare (command %u, window lba 0x%x, %u bytes)\n",
		  command, window, want);
	hdr->resid = (int)(hdr->dxfer_len - want);
	return 0;
}

static long rc_api_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg)
{
	struct rc_api_snapshot *snap;
	struct sg_io_hdr hdr;
	u8 cdb[16];
	u8 *pkt;
	long ret;

	if (cmd != SG_IO) {
		rc_printk(RC_WARN, "/dev/rc_api: unsupported ioctl 0x%x (expected SG_IO)\n",
			  cmd);
		return -ENOTTY;
	}
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (copy_from_user(&hdr, (void __user *)arg, sizeof(hdr)))
		return -EFAULT;
	if (hdr.interface_id != 'S') {
		rc_printk(RC_WARN, "/dev/rc_api: SG_IO interface id 0x%02x, expected 'S'\n",
			  (unsigned int)hdr.interface_id);
		return -EINVAL;
	}
	if (hdr.cmd_len < 1 || hdr.cmd_len > sizeof(cdb)) {
		rc_printk(RC_WARN, "/dev/rc_api: SG_IO cmd_len %u out of range\n",
			  hdr.cmd_len);
		return -EINVAL;
	}
	if (!hdr.cmdp || copy_from_user(cdb, hdr.cmdp, hdr.cmd_len))
		return -EFAULT;

	hdr.status = 0;
	hdr.masked_status = 0;
	hdr.msg_status = 0;
	hdr.host_status = 0;
	hdr.driver_status = 0;
	hdr.info = 0;
	hdr.resid = 0;
	hdr.duration = 0;
	hdr.sb_len_wr = 0;

	/* Zeroed up front: descriptor reads serve this buffer as-is.  The
	 * snapshot is heap-allocated too — it is ~2 KiB, and the kernel
	 * stack frame budget is 1 KiB (-Wframe-larger-than). */
	pkt = kvzalloc(RC_API_PACKET_BYTES, GFP_KERNEL);
	snap = kzalloc(sizeof(*snap), GFP_KERNEL);
	if (!pkt || !snap) {
		ret = -ENOMEM;
		goto out;
	}

	ret = rc_api_snapshot_get(snap);
	if (ret)
		goto out;

	switch (cdb[0]) {
	case RC_API_OP_INQUIRY:
		ret = rc_api_inquiry(&hdr, cdb);
		break;
	case RC_API_OP_READ_10:
		ret = rc_api_read(file, &hdr, cdb, snap, pkt);
		break;
	case RC_API_OP_WRITE_10:
		ret = rc_api_write(&hdr, cdb);
		break;
	default:
		ret = rc_api_reject(&hdr, RC_API_ASC_INVALID_OPCODE);
		break;
	}

	/*
	 * The vendor's char driver reports success as status == 1, not the
	 * Linux sg driver's 0x0 (captured on a successful INQUIRY); rcadm
	 * treats 0 as a failed command.  masked_status stays 0 on success.
	 */
	if (!ret && !hdr.status)
		hdr.status = 0x01;

out:
	kvfree(pkt);
	kfree(snap);
	if (ret) {
		rc_printk(RC_WARN, "/dev/rc_api: ioctl opcode 0x%02x failed: %ld\n",
			  cdb[0], ret);
		return ret;
	}
	rc_printk(RC_INFO, "/dev/rc_api: ioctl opcode 0x%02x ok, status 0x%02x resid %d\n",
		  cdb[0], hdr.status, hdr.resid);
	if (copy_to_user((void __user *)arg, &hdr, sizeof(hdr)))
		return -EFAULT;
	return 0;
}

static int rc_api_open(struct inode *inode, struct file *file)
{
	struct rc_api_session *sess;

	sess = kzalloc(sizeof(*sess), GFP_KERNEL);
	if (!sess)
		return -ENOMEM;
	sess->next_window = RC_API_WINDOW_FIRST;
	file->private_data = sess;
	return 0;
}

static int rc_api_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	file->private_data = NULL;
	return 0;
}

static const struct file_operations rc_api_fops = {
	.owner = THIS_MODULE,
	.open = rc_api_open,
	.release = rc_api_release,
	.unlocked_ioctl = rc_api_ioctl,
	.llseek = noop_llseek,
};

static struct miscdevice rc_api_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = RC_API_DEVICE_NAME,
	.fops = &rc_api_fops,
	.mode = 0600,
};

int rc_api_init(void)
{
	int ret;

	ret = misc_register(&rc_api_misc);
	if (ret) {
		rc_printk(RC_ERROR, "rc_api_init: failed to register /dev/%s: %d\n",
			  RC_API_DEVICE_NAME, ret);
		return ret;
	}
	rc_api_registered = true;
	rc_printk(RC_NOTE, "rc_api_init: /dev/%s registered (minor %d)\n",
		  RC_API_DEVICE_NAME, rc_api_misc.minor);
	return 0;
}

void rc_api_cleanup(void)
{
	if (!rc_api_registered)
		return;
	misc_deregister(&rc_api_misc);
	rc_api_registered = false;
	rc_printk(RC_NOTE, "rc_api_cleanup: /dev/%s unregistered\n",
		  RC_API_DEVICE_NAME);
}
