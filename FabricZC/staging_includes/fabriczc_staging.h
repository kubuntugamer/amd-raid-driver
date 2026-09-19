#ifndef __FABRICZC_STAGING_H__
#define __FABRICZC_STAGING_H__

#include <linux/version.h>

/* Map basic fixed-width types explicitly to bypass missing system headers */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned char      uint8_t;
typedef long long          s64;
typedef unsigned long      size_t;

/* Atomic Synchronization Primitive Forgery for Standalone Compiles */
typedef struct {
    volatile int counter;
} atomic_t;

#define FABRICZC_MAGIC_HEALTHY   0x48435a21
#define FABRICZC_MAX_DEVICES     8
#define FABRICZC_CHUNK_SECTORS   2048
#define FABRICZC_MAX_EXTENTS     16

struct fabriczc_container_header {
    uint64_t sequence_generation_id;
    uint64_t fletcher64_checksum;
    uint32_t container_state_magic;
    uint8_t  container_uuid;
    uint8_t  active_member_slot;
    uint8_t  total_active_disks;
    uint8_t  max_disk_boundary;
    uint8_t  reserved_padding;
    uint32_t extent_chunk_sectors;
} __attribute__((packed));

struct fabriczc_sgl_segment {
    u64 host_logical_sector;
    u64 target_buffer_length;
    u32 transaction_flags;
    u32 segment_id;
};

struct fabriczc_sgl_descriptor_vector {
    struct fabriczc_sgl_segment *segments;
    u32 total_segments;
    u32 active_vector_id;
};

struct fabriczc_linear_extent {
    u64 logical_start_sector;
    u64 extent_total_sectors;
    u64 physical_base_offset;
    u32 mapped_member_disk_idx;
    u32 active_extent_flags;
};

struct fabriczc_extent_table {
    struct fabriczc_linear_extent extents[FABRICZC_MAX_EXTENTS];
    u32 total_registered_extents;
    u32 active_table_id;
};

/* Phase 6 Additions: Asynchronous Ring Fault Tracking Parameters */
struct fabriczc_fault_registry {
    u64 last_recorded_timestamp;
    u32 total_timeout_aborts;
    u32 active_fault_flags;
};

struct fabriczc_subsystem_matrix {
    struct fabriczc_container_header master_hdr;
    void *member_bdevs[FABRICZC_MAX_DEVICES];
    struct fabriczc_extent_table extent_map;
    struct fabriczc_fault_registry fault_log[FABRICZC_MAX_DEVICES];
    u32 total_registered_cpus;
};

#endif /* __FABRICZC_STAGING_H__ */
