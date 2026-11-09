/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * AMD-RAID control-node ABI shared by the kernel bridge and rcraidXpert.
 *
 * The bridge is a privileged control plane, but the inventory/status reads
 * are safe for unprivileged diagnostics.  Keep this header layout-stable:
 * the packed structs are copied verbatim across the ioctl boundary.
 */

#ifndef AMD_RAID_UAPI_H
#define AMD_RAID_UAPI_H

#include <linux/types.h>
#include <linux/ioctl.h>

/* -------------------------------------------------------------------------
 * Live-volume control commands (magic 'R')
 * ------------------------------------------------------------------------- */

#define AMD_RAID_MAGIC 'R'

struct amd_resync_range {
	__u32 start_sector;
	__u32 end_sector;
};

/* Read a u32 status mask (RC_AMD_STATUS_*). */
#define AMD_IOCTL_GET_STATUS   _IOR(AMD_RAID_MAGIC, 1, __u32)
/* Ask the live resync scheduler to rebuild a parked member. */
#define AMD_IOCTL_START_RESYNC _IOW(AMD_RAID_MAGIC, 2, struct amd_resync_range)
/* Fail a live member by volume slot. */
#define AMD_IOCTL_FAIL_MEMBER  _IOW(AMD_RAID_MAGIC, 3, __u32)

/* -------------------------------------------------------------------------
 * Status bits returned by AMD_IOCTL_GET_STATUS
 *
 * Bit 0 means the control plane is up, not that the array is healthy.
 * Consumers must look at FAILED/DEGRADED before calling an array optimal.
 * ------------------------------------------------------------------------- */

#define RC_AMD_STATUS_LIVE      (1u << 0)	/* driver/control plane up   */
#define RC_AMD_STATUS_DEGRADED  (1u << 1)	/* serving with a failed member */
#define RC_AMD_STATUS_RESYNCING (1u << 2)	/* rebuild in flight         */
#define RC_AMD_STATUS_OVERLOAD  (1u << 3)	/* reserved                  */
#define RC_AMD_STATUS_FAULTS    (1u << 4)	/* member/media faults seen  */
#define RC_AMD_STATUS_FAILED    (1u << 5)	/* volume absent or cannot serve */

/* -------------------------------------------------------------------------
 * Hardware inventory (magic 'a')
 *
 * This is the payload rcraidXpert's dashboard reads.  Explicit padding and
 * packing keep the kernel and Qt layouts identical.
 * ------------------------------------------------------------------------- */

#define AMD_IOCTL_MAGIC 'a'

struct AmdDriveTelemetry {
	__u8  port_id;
	__u8  slot_id;
	__u8  _pad0[2];
	__u32 disk_index;
	__u64 total_sectors;
	__u8  is_assigned;
	__u8  _pad1[7];
} __attribute__((packed));

struct AmdArrayTelemetry {
	__u32 array_id;
	__u32 raid_level;
	__u32 stripe_sectors;
	__u32 _pad0;
	__u64 total_capacity_bytes;
	__u32 member_disk_count;
	__u8  member_indices[32];
	char  name[32];
	__u8  _pad1[4];
} __attribute__((packed));

struct AmdHardwarePayload {
	__u32 total_discovered_drives;
	__u32 total_discovered_arrays;
	struct AmdDriveTelemetry drives[64];
	struct AmdArrayTelemetry arrays[32];
} __attribute__((packed));

#define AMD_IOCTL_GET_HARDWARE_INVENTORY \
	_IOR(AMD_IOCTL_MAGIC, 0x30, struct AmdHardwarePayload)
#define AMD_IOCTL_COMMIT_NEW_ARRAY \
	_IOW(AMD_IOCTL_MAGIC, 0x31, struct AmdHardwarePayload)
#define AMD_IOCTL_DESTROY_ARRAY \
	_IOW(AMD_IOCTL_MAGIC, 0x32, struct AmdHardwarePayload)

#ifdef __cplusplus
#define AMD_RAID_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define AMD_RAID_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

AMD_RAID_STATIC_ASSERT(sizeof(struct AmdDriveTelemetry) == 24,
		       "AmdDriveTelemetry layout changed");
AMD_RAID_STATIC_ASSERT(sizeof(struct AmdArrayTelemetry) == 96,
		       "AmdArrayTelemetry layout changed");
AMD_RAID_STATIC_ASSERT(sizeof(struct AmdHardwarePayload) == 4616,
		       "AmdHardwarePayload layout changed");

#endif /* AMD_RAID_UAPI_H */
