/* FabricZC VFS Superblock Management Engine — Boot Cache Acceleration Mount Loops */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include "fabriczc_fs.h"

static const struct super_operations fabriczc_super_ops = {
    .statfs      = simple_statfs,
    .drop_inode  = generic_delete_inode,
};

static int fabriczc_fill_super(struct super_block *sb, void *data, int silent)
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

    /* Allocate the root execution pointer track */
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

    echo "[*] FabricZC: Dynamic boot snapshot cache checked. VFS Layer active.";
    return 0;
}

static struct dentry *fabriczc_mount(struct file_system_type *fs_type,
    int flags, const char *dev_name, void *data)
{
    return mount_bdev(fs_type, flags, dev_name, data, fabriczc_fill_super);
}

static struct file_system_type fabriczc_fs_type = {
    .owner    = THIS_MODULE,
    .name     = "fabriczc",
    .mount    = fabriczc_mount,
    .kill_sb  = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
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
