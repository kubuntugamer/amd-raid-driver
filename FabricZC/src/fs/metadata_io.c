/*
 * src/fs/metadata_io.c - FabricZC Layer 2 Superblock and Array Metadata I/O Subsystem (Fixed Flush)
 */
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <fabriczc_internal.h>
#include <fabriczc_common.h>

int fabriczc_verify_superblock(struct fabriczc_superblock *sb)
{
    if (unlikely(!sb)) return -EINVAL;
    if (sb->sb_magic != FABRICZC_SB_MAGIC) return -EILSEQ;
    return 0;
}

int fabriczc_read_metadata_block(u64 block_index, void *buffer_data)
{
    struct fabriczc_runtime_context *ctx = fabriczc_get_runtime_ctx();
    loff_t pos = block_index * 4096;
    if (unlikely(!buffer_data || !ctx || !ctx->bdev_file_ptr)) return -EINVAL;
    if (kernel_read(ctx->bdev_file_ptr, buffer_data, 4096, &pos) != 4096) return -EIO;
    return 0;
}

int fabriczc_flush_metadata_block(u64 block_index, void *buffer_data)
{
    struct fabriczc_runtime_context *ctx = fabriczc_get_runtime_ctx();
    struct bio *bio;
    struct page *page;
    if (unlikely(!buffer_data || !ctx || !ctx->bdev)) return -EINVAL;
    page = virt_to_page(buffer_data);
    if (!page) return -EFAULT;
    bio = bio_alloc(ctx->bdev, 1, REQ_OP_WRITE | REQ_SYNC | REQ_META, GFP_NOIO);
    if (!bio) return -ENOMEM;
    bio->bi_iter.bi_sector = block_index * 8;
    if (bio_add_page(bio, page, 4096, offset_in_page(buffer_data)) != 4096) {
        bio_put(bio); return -EIO;
    }
    return fabriczc_submit_raw_bio(bio);
}

int fabriczc_add_directory_entry(struct buffer_head *bh, const char *name, u32 target_inode, u8 file_type)
{
    struct fabriczc_dir_entry *de, *next_de;
    unsigned int offset = 0, name_len = strlen(name);
    unsigned int rec_len = ((sizeof(struct fabriczc_dir_entry) + name_len + 7) & ~7);
    
    if (unlikely(!bh || !name)) return -EINVAL;
    if (name_len > FABRICZC_NAME_LEN) return -ENAMETOOLONG;

    while (offset < 4096) {
        de = (struct fabriczc_dir_entry *)((char *)bh->b_data + offset);
        
        if (de->record_length_bytes == 0) {
            de->inode_number = target_inode;
            de->record_length_bytes = 4096 - offset;
            de->name_length = name_len;
            de->file_type = file_type;
            memcpy(de->file_name, name, name_len);
            
            /* FORCE CACHE FLUSH TO HARDWARE SECTORS IMMEDIATELY */
            mark_buffer_dirty(bh);
            sync_dirty_buffer(bh);
            return 0;
        }
        
        unsigned int actual_used_len = ((sizeof(struct fabriczc_dir_entry) + de->name_length + 7) & ~7);
        if (de->inode_number != 0 && (de->record_length_bytes - actual_used_len) >= rec_len) {
            unsigned int left_over = de->record_length_bytes - actual_used_len;
            de->record_length_bytes = actual_used_len;
            
            offset += actual_used_len;
            next_de = (struct fabriczc_dir_entry *)((char *)bh->b_data + offset);
            next_de->inode_number = target_inode;
            next_de->record_length_bytes = left_over;
            next_de->name_length = name_len;
            next_de->file_type = file_type;
            memcpy(next_de->file_name, name, name_len);
            
            /* FORCE CACHE FLUSH TO HARDWARE SECTORS IMMEDIATELY */
            mark_buffer_dirty(bh);
            sync_dirty_buffer(bh);
            return 0;
        }
        
        offset += de->record_length_bytes;
    }
    return -ENOSPC;
}

int fabriczc_mount_journal(struct super_block *sb, u64 journal_block)
{
    struct buffer_head *bh;
    struct fabriczc_journal_header *jh;
    if (unlikely(!sb || journal_block == 0)) return -EINVAL;
    bh = sb_bread(sb, journal_block);
    if (!bh) return -EIO;
    jh = (struct fabriczc_journal_header *)bh->b_data;
    if (jh->jh_magic != FABRICZC_JOURNAL_MAGIC) {
        memset(bh->b_data, 0, 4096);
        jh->jh_magic = FABRICZC_JOURNAL_MAGIC;
        jh->jh_block_size = 4096;
        jh->jh_sequence_num = 1;
        mark_buffer_dirty(bh);
        sync_dirty_buffer(bh);
    }
    brelse(bh);
    return 0;
}

int fabriczc_journal_replay(struct super_block *sb, u64 journal_block)
{
    struct buffer_head *bh;
    struct fabriczc_journal_header *jh;
    if (unlikely(!sb || journal_block == 0)) return -EINVAL;
    bh = sb_bread(sb, journal_block);
    if (!bh) return -EIO;
    jh = (struct fabriczc_journal_header *)bh->b_data;
    if (jh->jh_magic == FABRICZC_JOURNAL_MAGIC && jh->jh_head_offset != jh->jh_tail_offset) {
        jh->jh_head_offset = jh->jh_tail_offset;
        mark_buffer_dirty(bh);
        sync_dirty_buffer(bh);
    }
    brelse(bh);
    return 0;
}

struct fabriczc_transaction_context *fabriczc_trans_start(struct super_block *sb)
{
    struct fabriczc_transaction_context *tx;
    static atomic_t global_tx_sequence = ATOMIC_INIT(1000);
    if (unlikely(!sb)) return ERR_PTR(-EINVAL);
    tx = kmalloc(sizeof(struct fabriczc_transaction_context), GFP_NOFS);
    if (!tx) return ERR_PTR(-ENOMEM);
    tx->t_tx_id = atomic_inc_return(&global_tx_sequence);
    tx->t_status_flags = FABRICZC_TRANS_ACTIVE;
    tx->t_start_time_jiffies = jiffies;
    tx->t_blocks_modified = 0;
    spin_lock_init(&tx->t_lock);
    return tx;
}

int fabriczc_trans_commit(struct fabriczc_transaction_context *tx)
{
    unsigned long flags;
    if (unlikely(!tx)) return -EINVAL;
    spin_lock_irqsave(&tx->t_lock, flags);
    tx->t_status_flags = FABRICZC_TRANS_COMMITTED;
    spin_unlock_irqrestore(&tx->t_lock, flags);
    kfree(tx);
    return 0;
}

int fabriczc_trans_commit_to_journal(struct super_block *sb, struct fabriczc_transaction_context *tx, u64 journal_block)
{
    int ret;
    if (unlikely(!sb || !tx || journal_block == 0)) return -EINVAL;
    if (tx->t_status_flags != FABRICZC_TRANS_ACTIVE) return -EINVAL;
    ret = fabriczc_journal_append_entry(sb, journal_block, FABRICZC_JNL_OP_INODE, 0, NULL, 0);
    if (ret < 0) return ret;
    return fabriczc_trans_commit(tx);
}

int fabriczc_journal_append_entry(struct super_block *sb, u64 journal_block, u16 op_type, u64 target_blk, void *data, u16 len)
{
    struct buffer_head *hdr_bh;
    struct fabriczc_journal_header *jh;
    char *journal_data_ptr;
    u32 current_tail;
    if (unlikely(!sb || journal_block == 0)) return -EINVAL;
    hdr_bh = sb_bread(sb, journal_block);
    if (!hdr_bh) return -EIO;
    jh = (struct fabriczc_journal_header *)hdr_bh->b_data;
    current_tail = jh->jh_tail_offset;
    journal_data_ptr = (char *)hdr_bh->b_data + sizeof(struct fabriczc_journal_header) + current_tail;
    if (sizeof(struct fabriczc_journal_header) + current_tail + len + 16 > 4096) {
        current_tail = 0;
        journal_data_ptr = (char *)hdr_bh->b_data + sizeof(struct fabriczc_journal_header);
    }
    *((u16 *)journal_data_ptr) = op_type;
    *((u64 *)(journal_data_ptr + 2)) = target_blk;
    *((u16 *)(journal_data_ptr + 10)) = len;
    if (data && len > 0) memcpy(journal_data_ptr + 12, data, len);
    jh->jh_tail_offset = current_tail + len + 16;
    jh->jh_total_records++;
    jh->jh_sequence_num++;
    mark_buffer_dirty(hdr_bh);
    sync_dirty_buffer(hdr_bh);
    brelse(hdr_bh);
    return 0;
}

int fabriczc_find_and_allocate_inode(void *bitmap_page, unsigned int max_inodes_per_group) { return -ENOSPC; }
struct buffer_head *fabriczc_load_block_bitmap(struct super_block *sb, unsigned int block_group_idx, struct fabriczc_block_group_desc *desc) { return NULL; }
int fabriczc_execute_block_allocation(struct super_block *sb, struct fabriczc_block_group_desc *desc, unsigned int block_group_idx, unsigned int start_block) { return -ENOSPC; }
int fabriczc_sync_block_groups(struct super_block *sb, struct fabriczc_block_group_desc *caches, u32 group_count) { return 0; }
int fabriczc_sync_block_bitmaps(struct super_block *sb, struct fabriczc_block_group_desc *caches, u32 group_count) { return 0; }
u32 fabriczc_lookup_extent_block(void *header_page, u32 logical_block) { return 0; }
int fabriczc_insert_extent_descriptor(void *header_page, u32 logical_block, u32 block_len, u32 physical_start) { return 0; }
int fabriczc_sync_inode_to_disk(struct super_block *sb, struct fabriczc_block_group_desc *caches, u32 ino, struct fabriczc_inode *raw_inode) { return 0; }
int fabriczc_read_inode_from_disk(struct super_block *sb, struct fabriczc_block_group_desc *caches, u32 ino, struct fabriczc_inode *dest_inode) { return 0; }
int fabriczc_execute_block_release(struct super_block *sb, struct fabriczc_block_group_desc *desc, u64 absolute_block) { return 0; }
int fabriczc_execute_inode_release(struct super_block *sb, struct fabriczc_block_group_desc *desc, u32 ino) { return 0; }
int fabriczc_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create) { return 0; }
int fabriczc_allocate_file_extent(struct fabriczc_transaction_context *tx, void *header_page, u32 logical_block, u32 block_len, u32 physical_start) { return 0; }
void fabriczc_evict_inode(struct inode *inode) { truncate_inode_pages_final(&inode->i_data); clear_inode(inode); }
