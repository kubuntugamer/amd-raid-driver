#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <fabriczc_internal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chazz");
MODULE_DESCRIPTION("FabricZC Layer 2 - Filesystem & Experimental Metadata Container Layer");

static int __init fabriczc_fs_init(void) {
    struct fabriczc_runtime_context *ctx = fabriczc_get_runtime_ctx();
    pr_info("FabricZC: Initializing Layer 2 Filesystem Engine\n");
    if (ctx) {
        pr_info("FabricZC: Successfully linked to Layer 1 Core Core Engine Context\n");
    }
    return 0;
}

static void __exit fabriczc_fs_exit(void) {
    pr_info("FabricZC: Unregistering Layer 2 Filesystem Engine\n");
}

module_init(fabriczc_fs_init);
module_exit(fabriczc_fs_exit);
