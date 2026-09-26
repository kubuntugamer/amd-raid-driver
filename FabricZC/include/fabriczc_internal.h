/*
 * include/fabriczc_internal.h - Internal System Core Prototypes & Shared Context Definitions
 */

#ifndef FABRICZC_INTERNAL_H
#define FABRICZC_INTERNAL_H

#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/spinlock.h>

#define FABRICZC_MAX_LOCK_GROUPS 512

struct fabriczc_runtime_context {
    struct block_device *bdev;     /* Backing device hook target point link node */
    struct file *bdev_file_ptr;
    atomic_t active_ios;           /* Track active transaction references across boundaries */
    u32 operational_flags;         /* Runtime performance flag bitmask settings modifiers */
    spinlock_t allocation_locks[FABRICZC_MAX_LOCK_GROUPS]; /* Concurrency lock array */
};

/* Layer 1 Exported Symbol Functions */
struct fabriczc_runtime_context *fabriczc_get_runtime_ctx(void);
int fabriczc_submit_raw_bio(struct bio *bio);
int fabriczc_expose_disk(void);
int fabriczc_register_block_layer(int *major_num);
void fabriczc_unregister_block_layer(int major_num);
void fabriczc_trigger_power_state_drop(unsigned int power_level);

/* Layer 2 Exported Symbol Metadata Functions */
struct fabriczc_array_metadata;
struct fabriczc_superblock;
struct fabriczc_block_group_desc;
struct fabriczc_inode;

int fabriczc_verify_metadata(struct fabriczc_array_metadata *meta);
int fabriczc_verify_superblock(struct fabriczc_superblock *sb);
int fabriczc_find_and_allocate_inode(void *bitmap_page, unsigned int max_inodes_per_group);
int fabriczc_read_metadata_block(u64 block_index, void *buffer_data);
int fabriczc_flush_metadata_block(u64 block_index, void *buffer_data);

struct buffer_head *fabriczc_load_block_bitmap(struct super_block *sb, 
                                               unsigned int block_group_idx,
                                               struct fabriczc_block_group_desc *desc);

int fabriczc_execute_block_allocation(struct super_block *sb, 
                                      struct fabriczc_block_group_desc *desc,
                                      unsigned int block_group_idx,
                                      unsigned int start_block);

int fabriczc_sync_block_groups(struct super_block *sb, 
                              struct fabriczc_block_group_desc *caches, 
                              u32 group_count);

int fabriczc_sync_block_bitmaps(struct super_block *sb,
                               struct fabriczc_block_group_desc *caches,
                               u32 group_count);

int fabriczc_add_directory_entry(struct buffer_head *bh, const char *name, u32 target_inode, u8 file_type);
u32 fabriczc_lookup_extent_block(void *header_page, u32 logical_block);
int fabriczc_insert_extent_descriptor(void *header_page, u32 logical_block, u32 block_len, u32 physical_start);

int fabriczc_sync_inode_to_disk(struct super_block *sb,
                                struct fabriczc_block_group_desc *caches,
                                u32 ino,
                                struct fabriczc_inode *raw_inode);

int fabriczc_read_inode_from_disk(struct super_block *sb,
                                  struct fabriczc_block_group_desc *caches,
                                  u32 ino,
                                  struct fabriczc_inode *dest_inode);

int fabriczc_mount_journal(struct super_block *sb, u64 journal_block);
int fabriczc_journal_replay(struct super_block *sb, u64 journal_block);
int fabriczc_journal_append_entry(struct super_block *sb, u64 journal_block, u16 op_type, u64 target_blk, void *data, u16 len);


/* Transaction lifecycle operations exported from src/fs/metadata_io.c */
struct fabriczc_transaction_context;
extern struct fabriczc_transaction_context *fabriczc_trans_start(struct super_block *sb);

/* Transaction finalize operations exported from src/fs/metadata_io.c */
extern int fabriczc_trans_commit(struct fabriczc_transaction_context *tx);

/* Transaction-aware extent utilities exported from src/fs/metadata_io.c */
extern int fabriczc_allocate_file_extent(struct fabriczc_transaction_context *tx, void *header_page, u32 logical_block, u32 block_len, u32 physical_start);

/* In-memory mapping shortcuts exported from src/fs/fs_main.c */
extern void fabriczc_register_inode_cache(u32 ino, struct inode *inode);

/* Transaction log flushing operations exported from src/fs/metadata_io.c */
extern int fabriczc_trans_commit_to_journal(struct super_block *sb, struct fabriczc_transaction_context *tx, u64 journal_block);

/* Block reclamation operations exported from src/fs/metadata_io.c */
extern int fabriczc_execute_block_release(struct super_block *sb, struct fabriczc_block_group_desc *desc, u64 absolute_block);

/* Inode reclamation operations exported from src/fs/metadata_io.c */
extern int fabriczc_execute_inode_release(struct super_block *sb, struct fabriczc_block_group_desc *desc, u32 ino);

/* Transaction-wrapped staging helpers exported from src/fs/fs_main.c */
extern int fabriczc_stage_transactional_file(struct super_block *sb, u32 target_ino);

/* Cache lookaside query operations exported from src/fs/fs_main.c */
extern struct inode *fabriczc_find_inode_cache(u32 ino);

/* File data structural initializers exported from src/fs/fs_main.c */
extern void fabriczc_init_file_inode_ops(struct inode *inode);

/* Block allocation translation operations exported from src/fs/metadata_io.c */
extern int fabriczc_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create);
#endif /* FABRICZC_INTERNAL_H */

int fabriczc_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create);

int fabriczc_fs_init(void);
void fabriczc_fs_exit(void);

int fabriczc_submit_raw_bio(struct bio *bio);

int fabriczc_optimize_dir_lookup(void *bh_data, const char *name, u32 *out_ino);

void fabriczc_put_super(struct super_block *sb);
void fabriczc_discover_hardware_targets(void);
int fabriczc_expose_disk(void);
