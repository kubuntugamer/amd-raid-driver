/*
 * src/fs/fs_main.c - FabricZC VFS Entry (Coherent Track Sync)
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/fs_context.h>
#include <linux/slab.h>
#include <linux/math64.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>
#include <fabriczc_internal.h>
#include <fabriczc_common.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chazz");

extern void fabriczc_evict_inode(struct inode *inode);

static const struct file_operations fabriczc_dir_operations;
static const struct inode_operations fabriczc_dir_inode_operations;
static const struct super_operations fabriczc_super_ops;

static const struct inode_operations fabriczc_file_inode_operations;
static const struct file_operations fabriczc_file_operations;

static int fabriczc_fill_super(struct super_block *sb, struct fs_context *fc)
{
    struct inode *root;
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = 4096;
    sb->s_blocksize_bits = 12;
    sb->s_op = &fabriczc_super_ops;

    fabriczc_mount_journal(sb, 3);
    {
        void *scratch_page = kmalloc(4096, GFP_KERNEL);
        if (scratch_page) {
            fabriczc_read_metadata_block(3, scratch_page);
            fabriczc_flush_metadata_block(3, scratch_page);
            kfree(scratch_page);
        }
    }

    root = new_inode(sb);
    if (!root) return -ENOMEM;
    root->i_ino = 1;
    root->i_mode = S_IFDIR | 0755;
    root->i_op = &fabriczc_dir_inode_operations;
    root->i_fop = &fabriczc_dir_operations;

    sb->s_root = d_make_root(root);
    if (!sb->s_root) return -ENOMEM;
    return 0;
}

static int fabriczc_get_tree(struct fs_context *fc)
{
    return get_tree_bdev(fc, fabriczc_fill_super);
}

static const struct fs_context_operations fabriczc_context_ops = {
    .get_tree       = fabriczc_get_tree,
};

static int fabriczc_init_fs_context(struct fs_context *fc)
{
    fabriczc_discover_hardware_targets();
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

static int fabriczc_iterate(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);
    struct super_block *sb = inode->i_sb;
    struct buffer_head *bh;
    struct fabriczc_dir_entry *de;
    unsigned int offset = 0, record_index = 0;

    if (!dir_emit_dots(file, ctx)) return 0;
    bh = sb_bread(sb, 0);
    if (!bh) return -EIO;

    while (offset < 4096) {
        de = (struct fabriczc_dir_entry *)(bh->b_data + offset);
        if (de->record_length_bytes == 0) break;
        
        /* Evaluate entry indexes explicitly by adding a virtual dot offset shift modifier */
        if (de->inode_number > 0 && de->name_length > 0) {
            if ((record_index + 2) >= ctx->pos) {
                if (!dir_emit(ctx, de->file_name, de->name_length, de->inode_number, de->file_type)) {
                    break;
                }
                ctx->pos = (record_index + 2) + 1;
            }
            record_index++;
        }
        offset += de->record_length_bytes;
    }
    brelse(bh);
    return 0;
}

static int fabriczc_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    struct super_block *sb = dir->i_sb;
    struct fabriczc_transaction_context *tx;
    struct inode *inode;
    u32 allocated_ino = 2;
    struct buffer_head *bh;
    int ret;

    tx = fabriczc_trans_start(sb);
    if (IS_ERR(tx)) return PTR_ERR(tx);
    inode = new_inode(sb);
    if (!inode) { fabriczc_trans_commit(tx); return -ENOMEM; }

    inode_init_owner(idmap, inode, dir, mode);
    inode->i_blocks = 8;
    inode->i_ino = allocated_ino;
    inode->i_mode = mode;
    inode->i_op = &fabriczc_file_inode_operations;
    inode->i_fop = &fabriczc_file_operations;

    bh = sb_bread(sb, 0);
    if (!bh) { iput(inode); fabriczc_trans_commit(tx); return -EIO; }

    ret = fabriczc_add_directory_entry(bh, dentry->d_name.name, allocated_ino, DT_REG);
    if (ret < 0) { brelse(bh); iput(inode); fabriczc_trans_commit(tx); return ret; }
    
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    d_instantiate(dentry, inode);
    return fabriczc_trans_commit_to_journal(sb, tx, 3);
}

static struct dentry *fabriczc_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct super_block *sb = dir->i_sb;
    const char *name = dentry->d_name.name;
    struct buffer_head *bh;
    u32 ino = 0;
    struct inode *inode = NULL;

    bh = sb_bread(sb, 0);
    if (!bh) return ERR_PTR(-EIO);
    if (fabriczc_optimize_dir_lookup(bh->b_data, name, &ino) == 0) {
        inode = iget_locked(sb, ino);
        if (inode && (inode_state_read_once(inode) & I_NEW)) {
            inode->i_mode = S_IFREG | 0644;
            inode->i_op = &fabriczc_file_inode_operations;
            inode->i_fop = &fabriczc_file_operations;
            unlock_new_inode(inode);
        }
        brelse(bh);
        return d_splice_alias(inode, dentry);
    }
    brelse(bh);
    return d_splice_alias(NULL, dentry);
}

static const struct file_operations fabriczc_dir_operations = {
    .iterate_shared = fabriczc_iterate,
    .read           = generic_read_dir,
    .llseek         = generic_file_llseek,
};

static const struct inode_operations fabriczc_dir_inode_operations = {
    .lookup         = fabriczc_lookup,
    .create         = fabriczc_create,
};

static const struct super_operations fabriczc_super_ops = {
    .evict_inode    = fabriczc_evict_inode,
    .put_super      = fabriczc_put_super,
};

static const struct inode_operations fabriczc_file_inode_operations = {};
static const struct file_operations fabriczc_file_operations = {
    .llseek         = generic_file_llseek,
    .read_iter      = generic_file_read_iter,
    .write_iter     = generic_file_write_iter,
};

void fabriczc_put_super(struct super_block *sb) { if (sb) sync_filesystem(sb); }
int fabriczc_fs_init(void) { return register_filesystem(&fabriczc_fs_type); }
void fabriczc_fs_exit(void) { unregister_filesystem(&fabriczc_fs_type); }

int fabriczc_optimize_dir_lookup(void *bh_data, const char *name, u32 *out_ino)
{
    struct fabriczc_dir_entry *de = (struct fabriczc_dir_entry *)bh_data;
    unsigned int offset = 0;
    size_t name_len = strlen(name);
    while (offset < 4096) {
        if (de->record_length_bytes == 0) break;
        if (de->name_length == name_len && memcmp(de->file_name, name, name_len) == 0) {
            *out_ino = de->inode_number;
            return 0;
        }
        offset += de->record_length_bytes;
        de = (struct fabriczc_dir_entry *)((char *)bh_data + offset);
    }
    return -ENOENT;
}
