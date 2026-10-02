/*
 * src/core/main.c - Main Entry, Dynamic Discovery Scanner, & Delayed Work Watchdog Core
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <fabriczc_internal.h>
void fabriczc_discover_hardware_targets(void);
#include <fabriczc_common.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chazz");
MODULE_DESCRIPTION("FabricZC Layer 1 - Hardware & RAID Controller Interface");

struct fabriczc_runtime_context core_ctx;
static int fabriczc_major = 0;
static dev_t host_os_dev = 0;

static char *fabriczc_target = "/dev/rcraid0";
module_param_named(target, fabriczc_target, charp, 0444);
MODULE_PARM_DESC(target, "Backing block device for the FabricZC layer");


struct fabriczc_runtime_context *fabriczc_get_runtime_ctx(void)
{
    return &core_ctx;
}
EXPORT_SYMBOL_GPL(fabriczc_get_runtime_ctx);

int fabriczc_submit_raw_bio(struct bio *bio)
{
    if (unlikely(!core_ctx.bdev)) {
        bio_io_error(bio);
        return -ENODEV;
    }
    
    if (core_ctx.operational_flags & 0x00000003) {
        fabriczc_trigger_power_state_drop(0);
    }

    bio_set_dev(bio, core_ctx.bdev);
    submit_bio_noacct(bio);
    return 0;
}
EXPORT_SYMBOL_GPL(fabriczc_submit_raw_bio);


static void fabriczc_audit_host_os_identity(void)
{
    struct path path;
    if (kern_path("/", LOOKUP_FOLLOW, &path) == 0) {
        if (path.dentry && path.dentry->d_sb) {
            host_os_dev = path.dentry->d_sb->s_dev;
            pr_info("FabricZC: Dynamically mapped active Host OS drive identity at major:minor [%u:%u]\n",
                    MAJOR(host_os_dev), MINOR(host_os_dev));
        }
        path_put(&path);
    }
}

static void fabriczc_probe_device_path(const char *path_str)
{
    struct file *bdev_file;
    struct block_device *bdev;
    u8 *sector_buffer;
    loff_t pos = 0;
    ssize_t bytes_read;

    bdev_file = bdev_file_open_by_path(path_str, BLK_OPEN_READ | BLK_OPEN_WRITE, &core_ctx, NULL);
    if (IS_ERR(bdev_file))
        return;

    bdev = file_bdev(bdev_file);
    if (!bdev || !bdev->bd_disk) {
        bdev_fput(bdev_file);
        return;
    }

    if (bdev->bd_dev == host_os_dev) {
        bdev_fput(bdev_file);
        return;
    }

    sector_buffer = kmalloc(512, GFP_KERNEL);
    if (!sector_buffer) {
        bdev_fput(bdev_file);
        return;
    }

    bytes_read = kernel_read(bdev_file, sector_buffer, 512, &pos);
    if (bytes_read == 512) {
        /* Drop out and skip if this node matches our live parent Host OS device footprint */

        pr_info("FabricZC: Verified isolated target storage disk node [%s]\n", bdev->bd_disk->disk_name);
        core_ctx.bdev = bdev;
        core_ctx.bdev_file_ptr = bdev_file; bdev_file = NULL;
        pr_info("FabricZC: Successfully bound hardware target array pipeline to [%s] natively\n", bdev->bd_disk->disk_name);
        fabriczc_refresh_disk_capacity();
    }

    kfree(sector_buffer);
    if (!core_ctx.bdev) {
        bdev_fput(bdev_file);
    }
}

void fabriczc_discover_hardware_targets(void)
{
    if (core_ctx.bdev_file_ptr)
        return; /* already bound */
    pr_info("FabricZC: Launching dynamic un-hardcoded block storage discovery scan...\n");
    fabriczc_probe_device_path(fabriczc_target);
}

static int __init fabriczc_core_init(void)
{
    int ret, i;

    pr_info("FabricZC: Initializing Layer 1 Core Engine Setup\n");
    memset(&core_ctx, 0, sizeof(core_ctx));
    atomic_set(&core_ctx.active_ios, 0);

    for (i = 0; i < FABRICZC_MAX_LOCK_GROUPS; i++) {
        spin_lock_init(&core_ctx.allocation_locks[i]);
    }

    ret = fabriczc_register_block_layer(&fabriczc_major);
    if (ret < 0) {
        pr_err("FabricZC: Registration failed\n");
        return ret;
    }

    fabriczc_audit_host_os_identity();
    fabriczc_discover_hardware_targets();

    ret = fabriczc_fs_init();
    if (ret < 0) {
        fabriczc_unregister_block_layer(fabriczc_major);
        return ret;
    }

    return 0;
}

static void __exit fabriczc_core_exit(void)
{
    fabriczc_fs_exit();
    if (core_ctx.bdev_file_ptr) { bdev_fput(core_ctx.bdev_file_ptr); core_ctx.bdev_file_ptr = NULL; }
    fabriczc_unregister_block_layer(fabriczc_major);
    pr_info("FabricZC: Unregistering Layer 1 Core Engine Setup Complete\n");
}

module_init(fabriczc_core_init);
module_exit(fabriczc_core_exit);
