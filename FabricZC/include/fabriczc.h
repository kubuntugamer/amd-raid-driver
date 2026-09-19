#ifndef _FABRICZC_H
#define _FABRICZC_H
#include <linux/types.h>

#define FABRICZC_MAX_DEVICES 8
#define FABRICZC_EXTENT_SIZE_SECTORS 2048

struct fabriczc_container_header {
    __u64 sequence_generation_id;
    __u64 fletcher64_checksum;
    __u32 container_state_magic;     /* 0x48435a21 (!ZCH) or 0x44435a21 (!ZCD) */
    __u8  container_uuid;
    __u8  active_member_slot;        /* Index 0-7 */
    __u8  total_active_disks;
    __u8  max_disk_boundary;         /* Ceiling = 8 */
    __u8  reserved_padding;
    __u32 extent_chunk_sectors;    /* Hardcoded to 2048 / 1 MiB */
} __attribute__((packed));

#endif
