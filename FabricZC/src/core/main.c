/*
 * src/core/main.c - Main Entry and Linkage Registration Core
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <fabriczc_internal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chazz");
MODULE_DESCRIPTION("FabricZC Layer 1 - Hardware & RAID Controller Interface");

static struct fabriczc_runtime_context core_ctx;

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
    submit_bio_noacct(bio);
    return 0;
}
EXPORT_SYMBOL_GPL(fabriczc_submit_raw_bio);

static int __init fabriczc_core_init(void)
{
    pr_info("FabricZC: Initializing Layer 1 Core Engine Setup\n");
    memset(&core_ctx, 0, sizeof(core_ctx));
    atomic_set(&core_ctx.active_ios, 0);
    return 0;
}

static void __exit fabriczc_core_exit(void)
{
    pr_info("FabricZC: Unregistering Layer 1 Core Engine Setup\n");
}

module_init(fabriczc_core_init);
module_exit(fabriczc_core_exit);
