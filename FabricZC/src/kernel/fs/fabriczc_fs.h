/* FabricZC Hardware-Aware Solid-State Filesystem Include Header Map */
#ifndef _FABRICZC_FS_H
#define _FABRICZC_FS_H

#include <linux/fs.h>
#include <linux/types.h>

#define FABRICZC_FS_MAGIC      0x465A4346  /* "FZCF" Block Key */
#define FABRICZC_STRIPE_SIZE   4096        /* Full hardware erase block target */

/* Superblock metadata container matching spec checkpoint rules */
struct fabriczc_fs_sb_info {
    unsigned long total_blocks;
    unsigned long free_blocks;
    spinlock_t    parallel_zone_lock;
};

/* Decentralized Self-Describing Data Block Tracker */
struct fabriczc_block_descriptor {
    __le64 parent_directory;
    __le64 sequence_index;
    __le32 cryptographic_signature;
};

extern const struct inode_operations fabriczc_inode_ops;
extern const struct file_operations fabriczc_file_ops;

#endif
