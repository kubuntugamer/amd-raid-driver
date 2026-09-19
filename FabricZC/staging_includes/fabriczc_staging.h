#ifndef __FABRICZC_STAGING_H__
#define __FABRICZC_STAGING_H__

#define FABRICZC_MAGIC_HEALTHY   0x48435a21
#define FABRICZC_MAX_DEVICES     8
#define FABRICZC_CHUNK_SECTORS   2048

struct fabriczc_container_header {
    unsigned long long sequence_generation_id;
    unsigned long long fletcher64_checksum;
    unsigned int       container_state_magic;
    unsigned char      container_uuid;
    unsigned char      active_member_slot;
    unsigned char      total_active_disks;
    unsigned char      max_disk_boundary;
    unsigned char      reserved_padding;
    unsigned int       extent_chunk_sectors;
} __attribute__((packed));

struct fabriczc_sgl_segment {
    unsigned long long host_logical_sector;
    unsigned long long target_buffer_length;
    unsigned int       transaction_flags;
    unsigned int       segment_id;
};

struct fabriczc_sgl_descriptor_vector {
    struct fabriczc_sgl_segment *segments;
    unsigned int                 total_segments;
    unsigned int                 active_vector_id;
};

struct fabriczc_subsystem_matrix {
    struct fabriczc_container_header master_hdr;
    void *member_bdevs[FABRICZC_MAX_DEVICES];
    void *alloc_groups;
    unsigned int total_registered_cpus;
    void *administration_lock;
};
#endif
