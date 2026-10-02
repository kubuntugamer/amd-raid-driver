/*
 * src/core/block_dev.c - FabricZC Block Device Interface (Fixed Node Exposure)
 */
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <fabriczc_internal.h>
#include <fabriczc_common.h>

extern struct fabriczc_runtime_context core_ctx;
static struct gendisk *fabric_disk = NULL;

static int fabriczc_bdev_open(struct gendisk *disk, blk_mode_t mode) { return 0; }
static void fabriczc_bdev_release(struct gendisk *disk) {}

static void fabriczc_submit_bio(struct bio *bio)
{
    struct fabriczc_runtime_context *ctx = &core_ctx;
    atomic_inc(&ctx->active_ios);
    if (ctx->bdev) {
        bio_set_dev(bio, ctx->bdev);
        submit_bio_noacct(bio);
    } else {
        bio_io_error(bio);
    }
    atomic_dec(&ctx->active_ios);
}

static const struct block_device_operations fabriczc_bdev_ops = {
    .owner       = THIS_MODULE,
    .open        = fabriczc_bdev_open,
    .release     = fabriczc_bdev_release,
    .submit_bio  = fabriczc_submit_bio,
};

int fabriczc_register_block_layer(int *major_num)
{
    int ret;
    struct queue_limits lim = {};
    
    if (!major_num) return -EINVAL;

    *major_num = register_blkdev(0, "fabriczc");
    if (*major_num < 0) return *major_num;

    lim.logical_block_size = 4096;
    lim.physical_block_size = 4096;

    fabric_disk = blk_alloc_disk(&lim, NUMA_NO_NODE);
    if (IS_ERR(fabric_disk)) {
        unregister_blkdev(*major_num, "fabriczc");
        return PTR_ERR(fabric_disk);
    }

    fabric_disk->major = *major_num;
    fabric_disk->first_minor = 0;
    fabric_disk->minors = 1;
    fabric_disk->fops = &fabriczc_bdev_ops;
    fabric_disk->private_data = &core_ctx;
    snprintf(fabric_disk->disk_name, sizeof(fabric_disk->disk_name), "fabriczc0");
    set_capacity(fabric_disk, 0);

    /* Push the registered layout onto the live VFS subsystems namespace tree */
    ret = add_disk(fabric_disk);
    if (ret) {
        put_disk(fabric_disk);
        fabric_disk = NULL;
        unregister_blkdev(*major_num, "fabriczc");
        return ret;
    }

    return 0;
}

void fabriczc_refresh_disk_capacity(void)
{
    if (fabric_disk && core_ctx.bdev)
        set_capacity(fabric_disk, bdev_nr_bytes(core_ctx.bdev) >> 9);
}

void fabriczc_unregister_block_layer(int major_num)
{
    if (fabric_disk) {
        del_gendisk(fabric_disk);
        put_disk(fabric_disk);
        fabric_disk = NULL;
    }
    unregister_blkdev(major_num, "fabriczc");
}

static void __maybe_unused fabric_bdev_anchor_vars(void) { (void)fabriczc_bdev_ops; }
int fabriczc_expose_disk(void) { return 0; }
