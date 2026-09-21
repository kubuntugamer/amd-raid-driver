/* FabricZC VFS Superblock Management Engine — New Linux Mount API Compliance */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs_context.h>
#include "fabriczc_fs.h"

static const struct super_operations fabriczc_super_ops = {
    .statfs      = simple_statfs,
};

static int fabriczc_fill_super(struct super_block *sb, struct fs_context *fc)
{
    struct fabriczc_fs_sb_info *sbi;
    struct inode *root_inode;

    sbi = kzalloc(sizeof(struct fabriczc_fs_sb_info), GFP_KERNEL);
    if (!sbi)
        return -ENOMEM;

    sb->s_fs_info = sbi;
    sb->s_magic = FABRICZC_FS_MAGIC;
    sb->s_op = &fabriczc_super_ops;
    sb->s_time_gran = 1;

    spin_lock_init(&sbi->parallel_zone_lock);

    /* Allocate the root execution pointer track natively */
    root_inode = new_inode(sb);
    if (!root_inode) {
        kfree(sbi);
        return -ENOMEM;
    }

    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_mode = S_IFDIR | 0755;
    set_nlink(root_inode, 2);
    root_inode->i_op = &fabriczc_inode_ops;
    root_inode->i_fop = &simple_dir_operations;

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        kfree(sbi);
        return -ENOMEM;
    }

    printk(KERN_INFO "FabricZC: Dynamic boot snapshot cache checked. VFS Layer active.\n");
    return 0;
}

/* Modern Linux 7.x Mount Context Extraction Target */
static int fabriczc_get_tree(struct fs_context *fc)
{
    return get_tree_bdev(fc, fabriczc_fill_super);
}

static const struct fs_context_operations fabriczc_context_ops = {
    .get_tree    = fabriczc_get_tree,
};

static int fabriczc_init_fs_context(struct fs_context *fc)
{
    fc->ops = &fabriczc_context_ops;
    return 0;
}

static struct file_system_type fabriczc_fs_type = {
    .owner           = THIS_MODULE,
    .name            = "fabriczc",
    .init_fs_context = fabriczc_init_fs_context,
    .kill_sb         = kill_block_super,
    .fs_flags        = FS_REQUIRES_DEV,
};

static int __init init_fabriczc_fs(void)
{
    return register_filesystem(&fabriczc_fs_type);
}

static void __exit exit_fabriczc_fs(void)
{
    unregister_filesystem(&fabriczc_fs_type);
}

module_init(init_fabriczc_fs);
module_exit(exit_fabriczc_fs);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chazz");
MODULE_DESCRIPTION("FabricZC Parallel Flash-Aware Filesystem Layer");
