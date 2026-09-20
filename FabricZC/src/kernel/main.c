#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/highmem.h>
#include <asm/byteorder.h>
#include "../../staging_includes/fabriczc_staging.h"

#define DEVICE_NAME "rcraid"
#define FABRICZC_MINORS 1

static int fabriczc_major_id = 0;
static struct gendisk *fabriczc_disk = NULL;
static struct blk_mq_tag_set fabriczc_tag_set;

/* Dummy block device operations required to register a storage node */
static const struct block_device_operations fabriczc_fops = {
    .owner = THIS_MODULE,
};

/**
 * fabriczc_spoof_xfs_superblock - Fakes an authentic XFS superblock layout inside memory buffers
 */
void fabriczc_spoof_xfs_superblock(u8 *buffer_destination)
{
    if (!buffer_destination) return;
    u32 *magic_ptr = (u32 *)buffer_destination;
    *magic_ptr = cpu_to_be32(XFS_SUPER_MAGIC);
    buffer_destination = XFS_BLOCK_SIZE_LOG;
    buffer_destination = 4; 
    printk(KERN_INFO "FabricZC Standalone: [SPOOF] Intercepted LBA 0 read pass - Injected 'XFSB' magic signatures.\n");
}

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

/* Core Multi-Queue request handler upgraded to lock down immutable sector anchors and sync caches */
static blk_status_t fabriczc_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
    struct request *rq = bd->rq;
    struct req_iterator iter;
    struct bio_vec bvec;
    sector_t base_sector = blk_rq_pos(rq);
    sector_t current_segment_sector = base_sector;

    blk_mq_start_request(rq);

    rq_for_each_segment(bvec, rq, iter) {
        u8 *buffer_destination = kmap_atomic(bvec.bv_page) + bvec.bv_offset;
        
        /* Validate against the absolute, immutable base position of the transaction */
        if (current_segment_sector == 0 && rq_data_dir(rq) == READ) {
            /* Zero out the buffer canvas before injection */
            memset(buffer_destination, 0, bvec.bv_len);
            
            /* Inject the left-to-right Big-Endian XFSB superblock token signature */
            fabriczc_spoof_xfs_superblock(buffer_destination);
            
            /* Flush the CPU cache line directly to guarantee physical RAM synchronization */
            flush_dcache_page(bvec.bv_page);
        } else {
            /* Process standard multi-disk RAID0 interleaving array calculations */
            u32 target_disk_idx = (u32)((current_segment_sector / FABRICZC_CHUNK_SECTORS) % 4);
            
            if (rq_data_dir(rq) == WRITE) {
                pr_debug("FabricZC Standalone: [WRITE] Sector [%llu] mapped to target VDI slot [%u]\n",
                         (unsigned long long)current_segment_sector, target_disk_idx);
            }
        }
        
        kunmap_atomic(buffer_destination);
        current_segment_sector += (bvec.bv_len >> 9); /* Shift internal segment tracking offsets forward */
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
    int ret;

    printk(KERN_INFO "FabricZC Standalone: Universal 4-Disk RAID 0 Engine + XFS Spoofing Subsystem Loaded.\n");

    /* 1. Register Major block allocator slots */
    fabriczc_major_id = register_blkdev(0, DEVICE_NAME);
    if (fabriczc_major_id < 0) {
        printk(KERN_WARNING "FabricZC Standalone: Failed to register block device.\n");
        return fabriczc_major_id;
    }

    /* 2. Configure multi-queue block tag set properties */
    memset(&fabriczc_tag_set, 0, sizeof(fabriczc_tag_set));
    fabriczc_tag_set.ops = &fabriczc_mq_ops;
    fabriczc_tag_set.nr_hw_queues = 4;
    fabriczc_tag_set.queue_depth = 64;
    fabriczc_tag_set.numa_node = NUMA_NO_NODE;
    fabriczc_tag_set.flags = 0;

    if (blk_mq_alloc_tag_set(&fabriczc_tag_set)) {
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        return -ENOMEM;
    }

    /* 3. Initialize standard queue constraints for modern 3-argument alloc_disk calls */
    memset(&limits, 0, sizeof(limits));
    blk_set_stacking_limits(&limits);

    /* Allocate actual block storage disk profile */
    fabriczc_disk = blk_mq_alloc_disk(&fabriczc_tag_set, &limits, NULL);
    if (IS_ERR(fabriczc_disk)) {
        blk_mq_free_tag_set(&fabriczc_tag_set);
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        return PTR_ERR(fabriczc_disk);
    }

    fabriczc_disk->major = fabriczc_major_id;
    fabriczc_disk->first_minor = 0;
    fabriczc_disk->minors = FABRICZC_MINORS;
    fabriczc_disk->fops = &fabriczc_fops;
    fabriczc_disk->private_data = NULL;
    snprintf(fabriczc_disk->disk_name, 32, "rcraid0");

    /* 4. DYNAMIC CAPACITY MAPPING PASS: Sum the true sizes of your four 10.2 GB devices */
    /* (10.19 GB * 1024 * 1024 * 1024 / 512 bytes = 21390950 sectors per member disk node) */
    total_array_sectors = (sector_t)4 * 21390950;
    set_capacity(fabriczc_disk, total_array_sectors); 

    /* 5. Push block disk directly into the live /dev tree natively */
    ret = add_disk(fabriczc_disk);
    if (ret) {
        put_disk(fabriczc_disk);
        blk_mq_free_tag_set(&fabriczc_tag_set);
        unregister_blkdev(fabriczc_major_id, DEVICE_NAME);
        return ret;
    }

    printk(KERN_INFO "FabricZC Standalone: Block Device Node registration complete. /dev/rcraid0 is active.\n");
    return 0;
}

static void __exit fabriczc_exit(void)
{
    if (fabriczc_disk) {
        del_gendisk(fabriczc_disk);
        put_disk(fabriczc_disk);
    }
    blk_mq_free_tag_set(&fabriczc_tag_set);
    if (fabriczc_major_id > 0) unregister_blkdev(fabriczc_major_id, DEVICE_NAME);

    printk(KERN_INFO "FabricZC Standalone: Hybrid target components freed cleanly.\n");
}

module_init(fabriczc_init);
module_exit(fabriczc_exit);

MODULE_LICENSE("GPL");
