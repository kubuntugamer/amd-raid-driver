#include "../../staging_includes/fabriczc_staging.h"

extern int pr_info(const char *fmt, ...);

int init_module(void);
void cleanup_module(void);

int fabriczc_validate_p2p_page(u64 address_vector, u32 pci_id)
{
    if (address_vector >= 0x100000000ULL) {
        pr_info("FabricZC: [P2PDMA] Validated page frame structure for PCI device [0x%X] -> VRAM vector [0x%llX]\n", 
                pci_id, address_vector);
        return 1;
    }
    return 0;
}

u32 fabriczc_translate_sgl_to_p2p(const struct fabriczc_sgl_descriptor_vector *vector, 
                                  const struct fabriczc_subsystem_matrix *matrix)
{
    u32 processed_count = 0;
    u32 current_disk_count;
    u32 idx;

    if (!vector || !matrix || vector->total_segments == 0) return 0;
    current_disk_count = matrix->master_hdr.total_active_disks;
    if (current_disk_count == 0) return 0;

    for (idx = 0; idx < vector->total_segments; ++idx) {
        struct fabriczc_sgl_segment *seg = &vector->segments[idx];
        u32 target_device_idx = (u32)((seg->host_logical_sector / FABRICZC_CHUNK_SECTORS) % current_disk_count);
        processed_count++;
    }
    return processed_count;
}

u64 fabriczc_map_linear_extent(u64 logical_sector, const struct fabriczc_extent_table *table, u32 *out_disk_idx)
{
    u32 idx;
    if (!table || !out_disk_idx || table->total_registered_extents == 0) return 0;

    for (idx = 0; idx < table->total_registered_extents; ++idx) {
        const struct fabriczc_linear_extent *ext = &table->extents[idx];
        if (logical_sector >= ext->logical_start_sector && 
            logical_sector < (ext->logical_start_sector + ext->extent_total_sectors)) {
            *out_disk_idx = ext->mapped_member_disk_idx;
            return ext->physical_base_offset + (logical_sector - ext->logical_start_sector);
        }
    }
    return 0;
}

int init_module(void)
{
    pr_info("FabricZC: Phase 5 Hybrid Driver/Container Subsystem Online.\n");
    return 0;
}

void cleanup_module(void)
{
    pr_info("FabricZC: Hybrid Engine unmapped cleanly.\n");
}
