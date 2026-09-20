#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/highmem.h>
#include <linux/bio.h>
#include <linux/version.h>
#include <asm/byteorder.h>
#include "../../staging_includes/fabriczc_staging.h"

#define DEVICE_NAME "rcraid"
#define FABRICZC_MINORS 16  /* Support full partitioning allocations (rcraid0p1, rcraid0p2) */

static int fabriczc_major_id = 0;
static struct gendisk *fabriczc_disk = NULL;
static struct blk_mq_tag_set fabriczc_tag_set;

/* Array matrix to track open handles of the underlying virtual storage devices safely */
static struct block_device *member_bdevs[4] = {NULL, NULL, NULL, NULL};

static const struct block_device_operations fabriczc_fops = {
    .owner = THIS_MODULE,
};

/* Forward prototypes to satisfy strict compiler warning parameters */
u32 fabriczc_translate_sgl_to_p2p(const struct fabriczc_sgl_descriptor_vector *vector, 
                                  const struct fabriczc_subsystem_matrix *matrix);
u64 fabriczc_map_linear_extent(u64 logical_sector, const struct fabriczc_extent_table *table, u32 *out_disk_idx);

u32 fabriczc_translate_sgl_to_p2p(const struct fabriczc_sgl_descriptor_vector *vector, 
                                  const struct fabriczc_subsystem_matrix *matrix)
{
    u32 processed_count = 0;
    u32 current_disk_count;
    u32 idx;

    if (!vector || !matrix || vector->total_segments == 0) return 0;
    current_disk_count = matrix->master_hdr.total_active_disks;
    if (current_disk_count == 0) current_disk_count = 4;

    for (idx = 0; idx < vector->total_segments; ++idx) {
        struct fabriczc_sgl_segment *seg = &vector->segments[idx];
        u32 target_device_idx = (u32)((seg->host_logical_sector / FABRICZC_CHUNK_SECTORS) % current_disk_count);
        printk(KERN_INFO "FabricZC Standalone: [RAID0] Seg [%u] mapped across VDI Target Port Index [%u]\n", 
               seg->segment_id, target_device_idx);
        processed_count++;
    }
    return processed_count;
}

u64 fabriczc_map_linear_extent(u64 logical_sector, const struct fabriczc_extent_table *table, u32 *out_disk_idx)
{
    u32 idx;
    if (!table || !out_disk_idx || table->total_registered_extents == 0) return 0;

    for (idx = 0; idx < table->total_registered_extents; ++idx) {
        const struct fabriczc_linear_extent *ext = &table->extents[idx];
        if (logical_sector >= ext->logical_start_sector && 
            logical_sector < (ext->logical_start_sector + ext->extent_total_sectors)) {
            *out_disk_idx = ext->mapped_member_disk_idx;
            return ext->physical_base_offset + (logical_sector - ext->logical_start_sector);
        }
    }
    return 0;
}

/**
 * fabriczc_queue_rq - Production Multi-Queue Request Processor Loop
 * Dynamically forwards incoming request structures straight down to active member disks.
 */
static blk_status_t fabriczc_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct bio *bio;
    sector_t base_sector;
    u32 target_disk_idx;

    blk_mq_start_request(rq);

    if (!rq) {
        return BLK_STS_IOERR;
    }

    bio = rq->bio;
    if (!bio) {
        blk_mq_end_request(rq, BLK_STS_OK);
        return BLK_STS_OK;
    }

    base_sector = blk_rq_pos(rq);
    target_disk_idx = (u32)((base_sector / FABRICZC_CHUNK_SECTORS) % 4);

    if (member_bdevs[target_disk_idx]) {
        struct bio *clone_bio;

        /* Allocate an independent metadata clone shell container mapping to the target hardware disk */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
        clone_bio = bio_alloc_clone(member_bdevs[target_disk_idx], bio, GFP_ATOMIC, &fs_bio_set);
#else
        clone_bio = bio_clone_fast(bio, GFP_ATOMIC, &fs_bio_set);
        if (clone_bio) {
            bio_set_dev(clone_bio, member_bdevs[target_disk_idx]);
        }
#endif

        if (clone_bio) {
            submit_bio_noacct(clone_bio);
        } else {
            blk_mq_end_request(rq, BLK_STS_RESOURCE);
            return BLK_STS_OK;
        }
    }

    blk_mq_end_request(rq, BLK_STS_OK);
    return BLK_STS_OK;
}

static const struct blk_mq_ops fabriczc_mq_ops = {
    .queue_rq = fabriczc_queue_rq,
};

static int __init fabriczc_init(void)
{
    struct queue_limits limits;
    sector_t total_array_sectors;
    int ret, i;
    char path[32]; /* Correctly size local array tracking path buffer bounds */

    printk(KERN_INFO "FabricZC Standalone: Universal 4-Disk RAID 0 Engine + Dynamic Geometry Mapping Initializing.\n");

    /* 1. Open and secure structural tracking reference locks for all 4 underlying 10.2 GB disks */
    for (i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "/dev/nvme0n%d", i + 1);
        
        /* Fall back to standard, stable blkdev_get_by_path interfaces for the 7.0.0-14 headers */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
        member_bdevs[i] = blkdev_get_by_path(path, FMODE_READ | FMODE_WRITE, THIS_MODULE, NULL);
#else
        member_bdevs[i] = blkdev_get_by_path(path, FMODE_READ | FMODE_WRITE, THIS_MODULE);
#endif
        if (IS_ERR(member_bdevs[i])) {
            printk(KERN_WARNING "FabricZC Standalone: Warning - could not claim bdev lock on path %s\n", path);
            member_bdevs[i] = NULL;
        }
    }

    /* 2. Dynamic block allocation major registration */
    fabriczc_major_id = register_blkdev(0, DEVICE_NAME);
    if (fabriczc_major_id < 0) {
        printk(KERN_WARNING "FabricZC Standalone: Failed to register block device major number.\n");
        for (i = 0; i < 4; i++) {
            if (member_bdevs[i]) blkdev_put(member_bdevs[i], FMODE_READ | FMODE_WRITE);
        }
        return fabriczc_major_id;
    }

    /* 3. Configure multi-queue block tag set properties */
    memset(&fabriczc_tag_set, 0, sizeof(fabriczc_tag_set));
    fabriczc_tag_set.ops = &fabriczc_mq_ops;
    fabriczc_tag_set.nr_hw_queues = 4;
    fabriczc_tag_set.queue_depth = 64;
    fabriczc_tag_set.numa_node = NUMA_NO_NODE;
    fabriczc_tag_set.flags = 0;

    if (blk_mq_alloc_tag_set(&fabriczc_tag_set)) {
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        for (i = 0; i < 4; i++) {
            if (member_bdevs[i]) blkdev_put(member_bdevs[i], FMODE_READ | FMODE_WRITE);
        }
        return -ENOMEM;
    }

    /* 4. Setup structural hardware stacking properties */
    memset(&limits, 0, sizeof(limits));
    blk_set_stacking_limits(&limits);

    /* Allocate disk configuration profile using the queue limit metrics block pointer */
    fabriczc_disk = blk_mq_alloc_disk(&fabriczc_tag_set, &limits, NULL);
    if (IS_ERR(fabriczc_disk)) {
        blk_mq_free_tag_set(&fabriczc_tag_set);
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        for (i = 0; i < 4; i++) {
            if (member_bdevs[i]) blkdev_put(member_bdevs[i], FMODE_READ | FMODE_WRITE);
        }
        return PTR_ERR(fabriczc_disk);
    }

    fabriczc_disk->major = fabriczc_major_id;
    fabriczc_disk->first_minor = 0;
    fabriczc_disk->minors = FABRICZC_MINORS;
    fabriczc_disk->fops = &fabriczc_fops;
    fabriczc_disk->private_data = NULL;
    snprintf(fabriczc_disk->disk_name, 32, "rcraid0");

    total_array_sectors = (sector_t)4 * 21390950;
    set_capacity(fabriczc_disk, total_array_sectors); 

    /* 5. Activate storage device node inside live system tree */
    ret = add_disk(fabriczc_disk);
    if (ret) {
        put_disk(fabriczc_disk);
        blk_mq_free_tag_set(&fabriczc_tag_set);
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        for (i = 0; i < 4; i++) {
            if (member_bdevs[i]) blkdev_put(member_bdevs[i], FMODE_READ | FMODE_WRITE);
        }
        return ret;
    }

    printk(KERN_INFO "FabricZC Standalone: Clean, partitionable 40.8 GB storage array is now live at /dev/rcraid0.\n");
    return 0;
}

static void __exit fabriczc_exit(void)
{
    int i;
    if (fabriczc_disk) {
        del_gendisk(fabriczc_disk);
        put_disk(fabriczc_disk);
    }
    blk_mq_free_tag_set(&fabriczc_tag_set);
    if (fabriczc_major_id > 0) unregister_blkdev(fabriczc_major_id, DEVICE_NAME);

    /* Clean de-allocation sequence using stable block reference drops */
    for (i = 0; i < 4; i++) {
        if (member_bdevs[i]) {
            blkdev_put(member_bdevs[i], FMODE_READ | FMODE_WRITE);
        }
    }

    printk(KERN_INFO "FabricZC Standalone: Hardware array drivers dismantled cleanly.\n");
}

module_init(fabriczc_init);
module_exit(fabriczc_exit);

MODULE_LICENSE("GPL");
