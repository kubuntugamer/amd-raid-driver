/**
 * ==============================================================================
 * UNIFIED PROTOTYPES FOR ASYNC WORKER, PARITY MATH CORES, AND NESTED GEOMETRY
 * MODULE PATH: /home/chazz/amd-raid-driver/src/patch_prototypes.h
 * ==============================================================================
 */

#ifndef PATCH_PROTOTYPES_H
#define PATCH_PROTOTYPES_H

#include <linux/bio.h>
#include <linux/types.h>

/* Asynchronous Worker Hooks */
void rc_amd_queue_async_completion(struct bio *parent_bio, int total_mirrors, int status_flag);
int rc_amd_init_async_subsystem(void);
void rc_amd_exit_async_subsystem(void);

/* RAID 5/6 Parity Engine Hooks */
void rc_amd_generate_raid5_parity(uint8_t **data_buffers, uint8_t *parity_buffer, size_t block_len, int num_drives);
void rc_amd_generate_raid6_parity(uint8_t **data_buffers, uint8_t *p_buffer, uint8_t *q_buffer, size_t block_len, int num_drives);

/* RAID 10 Nested Geometry Matrix Hooks */
u64 rc_amd_map_nested_raid10(u64 sector_lba, u32 chunk_sectors, int *target_member, int num_drives);

void rc_amd_route_io_nested_raid10(u64 *lba, int *mbr, u32 chunk_sectors, int num_drives);
blk_status_t rc_amd_handle_transient_retry(struct bio *bio, int retry_count, int max_retries, int device_status);
int rc_amd_init_ioctl_bridge(void);
void rc_amd_exit_ioctl_bridge(void);
#endif /* PATCH_PROTOTYPES_H */

/* Multi-Format Distributed Parity Layout Geometry Extensions */
u64 rc_amd_map_distributed_raid5(u64 sector_lba, u32 chunk_sectors, int *target_member, int *parity_member, int num_drives);
void rc_amd_route_io_distributed_raid5(u64 *lba, int *mbr, int *parity_mbr, u32 chunk_sectors, int num_drives);
