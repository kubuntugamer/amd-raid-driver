/*
 * FabricZC Storage Stack - Module-to-Module Exported Interface
 * Core file linkage boundary tracking.
 */

#ifndef __FABRICZC_INTERNAL_H__
#define __FABRICZC_INTERNAL_H__

#include <linux/types.h>
#include <linux/blkdev.h>

struct fabriczc_runtime_context {
    struct block_device *bdev;
    atomic_t active_ios;
    bool expansion_in_progress;
    u32 operational_flags;
};

extern struct fabriczc_runtime_context *fabriczc_get_runtime_ctx(void);
extern int fabriczc_submit_raw_bio(struct bio *bio);
extern void fabriczc_trigger_power_state_drop(unsigned int power_level);

#endif /* __FABRICZC_INTERNAL_H__ */
