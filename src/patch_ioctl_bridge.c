/**
 * ==============================================================================
 * UPSTREAM PATCH VECTOR: HIGHPOINT-STYLE IOCTL MANAGEMENT CONTROLLER LAYER
 * SOURCE ARCHITECTURE TARGET: kubuntugamer/amd-raid-driver / src/patch_ioctl_bridge.c
 * ==============================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "patch_prototypes.h"

#define AMD_RAID_MAGIC 'R'
#define AMD_IOCTL_GET_STATUS   _IOR(AMD_RAID_MAGIC, 1, uint32_t)
#define AMD_IOCTL_START_RESYNC _IOW(AMD_RAID_MAGIC, 2, uint32_t)
#define AMD_IOCTL_FAIL_MEMBER  _IOW(AMD_RAID_MAGIC, 3, uint32_t)

static int amd_ctl_major_number;
#define DEVICE_NODE_NAME "amd_ctl0"

/**
 * Core Core IOCTL Execution Router Matrix
 * Bridges user-space unprivileged requests directly to bare-metal kernel actions.
 */
static long amd_raid_ioctl_multiplexer(struct file *file, unsigned int cmd, unsigned long arg)
{
    uint32_t target_drive_index;
    uint32_t resync_start_sector;
    uint32_t resync_end_sector;
    struct rc_resync_progress progress;
    long ret;

    switch (cmd) {
    case AMD_IOCTL_GET_STATUS:
        /*
         * Live engine mask rather than a hardcoded "healthy" constant:
         * bit 0 is RC_AMD_STATUS_LIVE, so an unmodified reader still
         * sees 0x01 on a healthy array while degradation, an in-flight
         * rebuild, resource pressure and media faults become visible.
         */
        {
            uint32_t status_mask = rc_amd_get_status();

            if (copy_to_user((void __user *)arg, &status_mask, sizeof(status_mask))) {
                return -EFAULT;
            }
            pr_info("rcraid_ctl: dispatched status mask 0x%08x (%d inflight mirror bio(s), %d failed member(s))\\n",
                    status_mask, rc_amd_inflight_contexts(),
                    rc_amd_failed_member_count());
        }
        break;

    case AMD_IOCTL_START_RESYNC:
        /*
         * Argument is { start_sector, end_sector }.  The request is handed
         * to kamd_resync and this returns immediately: the rebuild owns
         * its own thread, so an ioctl caller never waits on the array and
         * never blocks the mirror completion pump.
         */
        if (copy_from_user(&resync_start_sector, (void __user *)arg,
                           sizeof(resync_start_sector))) {
            return -EFAULT;
        }
        if (copy_from_user(&resync_end_sector,
                           (void __user *)arg + sizeof(resync_start_sector),
                           sizeof(resync_end_sector))) {
            return -EFAULT;
        }

        pr_warn("rcraid_ctl: user-space requested resync of sectors %u..%u\\n",
                resync_start_sector, resync_end_sector);
        ret = rc_amd_resync_request(resync_start_sector, resync_end_sector,
                                    256 /* 128 KiB chunks, matches the
                                         * volume layer's resync stride */,
                                    -1 /* survivor chosen by the media layer */,
                                    -1 /* routed per chunk */);
        if (ret) {
            pr_err("rcraid_ctl: resync request rejected: %ld\\n", ret);
            return ret;
        }
        break;

    case AMD_IOCTL_FAIL_MEMBER:
        if (copy_from_user(&target_drive_index, (void __user *)arg, sizeof(target_drive_index))) {
            return -EFAULT;
        }
        pr_alert("rcraid_ctl: user-space command forcing eviction of member index %u\\n",
                 target_drive_index);

        /* Latches the slot failed: rebuild routing avoids it and the
         * status mask reports the array degraded. */
        ret = rc_amd_fail_member(target_drive_index);
        if (ret) {
            pr_err("rcraid_ctl: member index %u out of range\\n", target_drive_index);
            return ret;
        }

        /* A rebuild aimed at that slot is meaningless now — stop it
         * rather than let it fail one chunk at a time. */
        rc_amd_get_resync_progress(&progress);
        if (progress.state == RC_RESYNC_RUNNING &&
            progress.target_member == (int)target_drive_index) {
            pr_warn("rcraid_ctl: cancelling rebuild writing to evicted member %u\\n",
                    target_drive_index);
            rc_amd_resync_cancel();
        }
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

/* Bind file system dispatch matrix operation targets */
static const struct file_operations amd_ctl_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = amd_raid_ioctl_multiplexer,
    .compat_ioctl = amd_raid_ioctl_multiplexer,
};

/**
 * Module Initialization Integration Entries
 */
int rc_amd_init_ioctl_bridge(void)
{
    amd_ctl_major_number = register_chrdev(0, DEVICE_NODE_NAME, &amd_ctl_fops);
    if (amd_ctl_major_number < 0) {
        pr_err("rcraid_ctl: Failed to allocate character major character device registration matrix.\\n");
        return amd_ctl_major_number;
    }
    pr_info("rcraid_ctl: HighPoint-style control interface registered. Assigned Major ID: %d\\n", amd_ctl_major_number);
    return 0;
}

/**
 * Module Cleanup Elimination Entries
 */
void rc_amd_exit_ioctl_bridge(void)
{
    if (amd_ctl_major_number >= 0) {
        unregister_chrdev(amd_ctl_major_number, DEVICE_NODE_NAME);
    }
}
