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

/* Freestanding Primitive Type Alignment Definition */
typedef unsigned long      size_t;

/* Atomic Synchronization Primitive Forgery for Standalone Compiles */
typedef struct {
    volatile int counter;
} atomic_t;

#define FABRICZC_MAGIC_HEALTHY   0x48435a21
#define FABRICZC_MAX_DEVICES     8
#define FABRICZC_CHUNK_SECTORS   2048

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

struct fabriczc_allocation_group {
    u32 group_index;
    void *poll_kthread;
    void *event_wait_queue;
};

struct fabriczc_subsystem_matrix {
    struct fabriczc_container_header master_hdr;
    void *member_bdevs[FABRICZC_MAX_DEVICES];
    struct fabriczc_allocation_group *alloc_groups;
    u32 total_registered_cpus;
    void *administration_lock;
};

/* Phase 3: P2PDMA Hardware Routing Mapping Structures */
struct fabriczc_p2p_mapping {
    u64 pcie_device_vram_address;
    u32 target_pci_device_id;
    u32 page_allocation_status;
    u8  is_p2p_capable;
    u8  channel_bus_alignment_padding;
};

/* Expand the main system matrix tracking parameters to hold hardware maps */
struct fabriczc_p2p_engine {
    struct fabriczc_p2p_mapping active_mappings[FABRICZC_MAX_DEVICES];
    atomic_t total_p2p_allocated_pages;
};

#endif
