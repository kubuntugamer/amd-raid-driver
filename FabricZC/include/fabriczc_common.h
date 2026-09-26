/*
 * FabricZC Storage Stack - Shared Layout Definitions
 * 1-to-8 Dynamic Disk Expansion and Layout Limits
 */

#ifndef __FABRICZC_COMMON_H__
#define __FABRICZC_COMMON_H__

#include <linux/types.h>

#define FABRICZC_MIN_DISKS      1
#define FABRICZC_MAX_DISKS      8
#define FABRICZC_CHUNK_SIZE     (64 * 1024) /* 64KB Stripe Chunks */
#define FABRICZC_MAGIC          0x465A434D  /* "FZCM" Signature Magic */

/* Metadata disk geometry description layout */
struct fabriczc_disk_geometry {
    u32 disk_index;
    u64 sector_start;
    u64 total_sectors;
    u8  disk_uuid;
    u32 operational_status;
};

/* Master layout configuration tracking table */
struct fabriczc_array_metadata {
    u32 magic_number;        /* Signature verification */
    u32 active_disk_count;   /* Dynamic scaling: 1 scaled up to 8 */
    u64 logical_array_size;  /* Total expanded export volume size */
    struct fabriczc_disk_geometry devices[FABRICZC_MAX_DISKS];
    u64 generation_sequence; /* Crash resilience tracking index */
    u32 checksum;            /* Structural integrity validation check */
};

#endif /* __FABRICZC_COMMON_H__ */

#define FABRICZC_SB_MAGIC       0x465A4353  /* "FZCS" Superblock Magic */
#define FABRICZC_STATE_CLEAN    0x00000001
#define FABRICZC_STATE_DIRTY    0x00000002

/* Master filesystem layout block tracking structures */
struct fabriczc_superblock {
    u32 sb_magic;             /* Filesystem tracking signature */
    u32 sb_state;             /* Clean/Dirty partition shutdown state flags */
    u64 block_count;          /* Total logical filesystem blocks available */
    u64 free_blocks;          /* Total unallocated data block units */
    u32 block_size_bytes;     /* Allocation cluster block layout (e.g., 4096) */
    u8  fs_uuid[16];          /* Unique cryptographic volume tracking identifier */
    s64 last_mount_time;      /* UNIX epoch timestamp registration marker */
    u32 sb_checksum;          /* Cryptographic structure validation hash checks */
};

#define FABRICZC_BITS_PER_BLOCK (4096 * 8) /* 32,768 clusters tracked per bitmap block */

/* On-disk tracking layout structure for block group allocation descriptions */
struct fabriczc_block_group_desc {
    u32 bg_bitmap_block;      /* Physical block sector mapping index for the allocation bitmap */
    u32 bg_inode_table_block; /* Starting block tracking index for the local inode array metadata */
    u16 bg_free_blocks_count; /* Total remaining available block units in this distinct zone */
    u16 bg_free_inodes_count; /* Total remaining available inode structures in this distinct zone */
    u32 bg_checksum;          /* Boundary integrity validation signature block hash check */
};

#define FABRICZC_N_DIRECT_BLOCKS 12
#define FABRICZC_FT_REG_FILE     1
#define FABRICZC_FT_DIR          2

/* Master on-disk storage layout representation for file nodes */
struct fabriczc_inode {
    u16 i_mode;               /* Standard type and permission access bitmask flags */
    u16 i_links_count;        /* Total tracking directory hard-link references */
    u32 i_uid;                /* Owner user tracking identification index */
    u32 i_gid;                /* Group tracking identification index */
    u64 i_size_bytes;         /* Complete structural file capacity payload depth in bytes */
    u64 i_blocks_allocated;   /* Count of assigned 512-byte structural sectors */
    u32 i_generation;         /* Volume file modification generation tracking index */
    u32 i_direct_blocks[FABRICZC_N_DIRECT_BLOCKS]; /* Map pointers to direct 4KB clusters */
    u32 i_indirect_block;     /* Map pointer targeting a secondary single-indirect index table */
    u32 i_checksum;           /* Individual item structure integrity verification signature */
};

#define FABRICZC_NAME_LEN 255

/* Master on-disk storage layout representation for directory records */
struct fabriczc_dir_entry {
    u32 inode_number;             /* Numeric metadata node tracking index for this file record */
    u16 record_length_bytes;      /* Total packing alignment size step size for this record entry */
    u8  name_length;              /* True length tracking value for the string path payload */
    u8  file_type;                /* Directory layout type marker (File, Directory, etc.) */
    char file_name[FABRICZC_NAME_LEN]; /* Aligned string data array housing the actual name */
};

#define FABRICZC_EXTENT_MAGIC   0xE17E6463 /* "EXDC" Extent Descriptor Container Magic */

/* Master compact on-disk layout representing a contiguous span of data block allocations */
struct fabriczc_extent_descriptor {
    u32 ee_block;             /* First logical file block cluster offset tracked by this entry */
    u32 ee_len;               /* Count of contiguous 4KB block segments mapped inside this run */
    u32 ee_start_block;       /* Absolute physical cluster block tracking location index on the drive */
    u32 ee_flags;             /* Allocation state context bitmask attributes (e.g., initialized vs unwritten) */
};

/* Header structure tracking a multi-extent file data tree container block */
struct fabriczc_extent_header {
    u16 eh_magic;             /* Structural layout validity signature verification flag (FABRICZC_EXTENT_MAGIC) */
    u16 eh_entries;           /* Current count of valid active descriptor structures packed inside this block */
    u16 eh_max_entries;       /* Maximum data payload entry capacity boundary ceiling of this structural container */
    u16 eh_depth;             /* Structural tree depth level indicator (0 for leaf nodes mapping data directly) */
    u32 eh_generation;        /* Individual tree modification generation tracking signature check hash value */
};

#define FABRICZC_JOURNAL_MAGIC  0x4A4E4C4D /* "JNLM" Journal Master Magic */
#define FABRICZC_JNL_OP_ALLOC   1
#define FABRICZC_JNL_OP_FREE    2
#define FABRICZC_JNL_OP_INODE   3

/* Layout description tracking an individual atomic transaction record entry */
struct fabriczc_journal_entry {
    u32 je_tx_id;              /* Global unique transaction tracking transaction group sequence identifier */
    u16 je_operation_type;    /* Structural modification opcode descriptor (ALLOC, FREE, INODE) */
    u16 je_data_len;           /* True payload payload tracking capacity offset size in bytes */
    u64 je_target_block;       /* Main disk destination sector cluster address being modified */
    u8  je_payload[64];        /* Aligned in-place raw byte array housing transient field changes */
    u32 je_crc32;              /* Individual transaction transaction boundary integrity hash validation check */
};

/* Header layout pinning down the circular transaction journal state configurations */
struct fabriczc_journal_header {
    u32 jh_magic;             /* Structural layout validity signature verification flag (FABRICZC_JOURNAL_MAGIC) */
    u32 jh_block_size;        /* Allocation boundary tracking configuration size of the log blocks */
    u64 jh_total_records;     /* Historical scalar tracker logging cumulative committed records */
    u32 jh_head_offset;       /* Active structural index offset pointer tracing the oldest log entry */
    u32 jh_tail_offset;       /* Active structural index offset pointer tracing the current append slot */
    u32 jh_sequence_num;      /* Master incrementing transaction group sequence identifier tracking number */
};

#define FABRICZC_TRANS_ACTIVE   0x00000001
#define FABRICZC_TRANS_COMMITTED 0x00000002

/* Master in-memory tracking handle grouping atomic metadata mutations */
struct fabriczc_transaction_context {
    u32 t_tx_id;              /* Global unique transaction group sequence identifier tracking number */
    u32 t_status_flags;       /* Active state flag tracking attributes (ACTIVE vs COMMITTED) */
    u64 t_start_time_jiffies; /* Kernel temporal timestamp tracking marker marking transaction creation */
    u16 t_blocks_modified;    /* Cumulative count of individual data blocks altered within this transaction context */
    spinlock_t t_lock;        /* Internal synchronization spinlock safeguarding internal structure deltas */
};


struct fabriczc_extent {
    u32 ee_block;         /* First logical block extent covers */
    u32 ee_len;           /* Number of blocks covered by extent */
    u32 ee_start_hi;      /* High 16 bits of physical block start */
    u32 ee_start_lo;      /* Low 32 bits of physical block start */
};
