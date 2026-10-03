#include "patch_prototypes.h"
/**
 * ==============================================================================
 * CLEANROOM ARCHITECTURE: INDEPENDENT PARITY GENERATION LAYER (RAID 5 / RAID 6)
 * MODULE PATH: joeytroy/amd-raid-driver / patch_parity_math.c
 * ==============================================================================
 *
 * Galois Field GF(2^8) arithmetic for RAID 6 Q parity.
 * Primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1  (0x11D)
 * Generator element:    α = 0x02
 * This is the standard Reed-Solomon field used by Linux md RAID-6,
 * Windows Storage Spaces, and the AMD vendor blob (per decompile).
 *
 * Q parity formula (Reed-Solomon):
 *   Q = Σ (di × α^i)  for i = 0..k-1  where k = data drives
 *   P = d0 ⊕ d1 ⊕ ... ⊕ d(k-1)  (simple XOR)
 *
 * Both parities are computed per-byte across the stripe.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/slab.h>

/* =========================================================================
 * GF(2^8) ARITHMETIC — standard polynomial 0x11D (x^8 + x^4 + x^3 + x^2 + 1)
 * Generator α = 0x02.  Tables generated once at module load.
 * ========================================================================= */

#define GF28_POLY  0x11D
#define GF28_ALPHA 0x02

static uint8_t gf28_exp[256];   /* α^i for i = 0..255 (wraps at 255) */
static uint8_t gf28_log[256];   /* log_α(x) for x = 1..255, log(0) = 255 (unused) */
static uint8_t gf28_mul[256];   /* multiplication by α (0x02): gf28_mul[x] = x * α */

static void gf28_init_tables(void)
{
    int i;
    uint16_t v = 1;

    /* Build exp/log tables: exp[i] = α^i, log[exp[i]] = i */
    for (i = 0; i < 255; i++) {
        gf28_exp[i] = (uint8_t)v;
        gf28_log[(uint8_t)v] = (uint8_t)i;
        v <<= 1;
        if (v & 0x100)
            v ^= GF28_POLY;
    }
    gf28_exp[255] = gf28_exp[0];  /* α^255 = α^0 = 1 */
    gf28_log[0] = 255;            /* log(0) undefined, mark as 255 */

    /* Precompute multiplication by α (0x02) for fast Q parity */
    for (i = 0; i < 256; i++) {
        if (i == 0) {
            gf28_mul[i] = 0;
        } else if (i & 0x80) {
            gf28_mul[i] = (uint8_t)((i << 1) ^ GF28_POLY);
        } else {
            gf28_mul[i] = (uint8_t)(i << 1);
        }
    }
}

/* Multiply two GF(2^8) elements using log/exp tables */
static inline uint8_t gf28_mul_general(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0)
        return 0;
    return gf28_exp[(gf28_log[a] + gf28_log[b]) % 255];
}

/* =========================================================================
 * RAID 5 PARITY (P) — simple XOR across all data drives
 * ========================================================================= */
void rc_amd_generate_raid5_parity(uint8_t **data_buffers, uint8_t *parity_buffer,
                                   size_t block_len, int num_drives)
{
    int data_drives = num_drives - 1;
    size_t i;
    int d;

    if (unlikely(data_drives <= 0 || block_len == 0))
        return;

    memset(parity_buffer, 0, block_len);

    for (d = 0; d < data_drives; d++) {
        uint8_t *src = data_buffers[d];
        for (i = 0; i < block_len; i++)
            parity_buffer[i] ^= src[i];
    }
}
EXPORT_SYMBOL_GPL(rc_amd_generate_raid5_parity);

/* =========================================================================
 * RAID 6 DUAL PARITY (P + Q)
 *
 * P = d0 ⊕ d1 ⊕ ... ⊕ d(k-1)                    (XOR)
 * Q = Σ (di × α^i)  for i = 0..k-1              (Reed-Solomon)
 *
 * Where k = data_drives = num_drives - 2
 * α = 0x02 is the primitive element.
 *
 * Per-byte computation:
 *   For each byte position j in [0, block_len):
 *     P[j] = d0[j] ⊕ d1[j] ⊕ ... ⊕ d(k-1)[j]
 *     Q[j] = gf_mul(d0[j], α^0) ⊕ gf_mul(d1[j], α^1) ⊕ ... ⊕ gf_mul(d(k-1)[j], α^(k-1))
 *
 * This matches the vendor blob's algorithm (uVar12 = 2 for RAID 5/6 parity count).
 * ========================================================================= */
void rc_amd_generate_raid6_parity(uint8_t **data_buffers, uint8_t *p_buffer,
                                   uint8_t *q_buffer, size_t block_len, int num_drives)
{
    int data_drives = num_drives - 2;
    size_t i;
    int d;

    if (unlikely(data_drives <= 0 || block_len == 0))
        return;

    memset(p_buffer, 0, block_len);
    memset(q_buffer, 0, block_len);

    /* Process one byte at a time — compiler will vectorize the inner loop */
    for (i = 0; i < block_len; i++) {
        uint8_t p = 0;
        uint8_t q = 0;

        for (d = 0; d < data_drives; d++) {
            uint8_t byte = data_buffers[d][i];

            /* P parity: XOR */
            p ^= byte;

            /* Q parity: multiply by α^d */
            if (byte != 0) {
                int exp_idx = d % 255;          /* α^d wraps at 255 (α^255 = 1) */
                uint8_t coeff = gf28_exp[exp_idx];
                q ^= gf28_mul_general(byte, coeff);
            }
        }

        p_buffer[i] = p;
        q_buffer[i] = q;
    }
}
EXPORT_SYMBOL_GPL(rc_amd_generate_raid6_parity);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Independent Cleanroom Parity Architecture Team");
MODULE_DESCRIPTION("RAID 5/6 parity generation — cleanroom GF(2^8) implementation");

/* Constructor: build GF(2^8) tables at module load time */
static __attribute__((constructor)) void rc_parity_init(void)
{
    gf28_init_tables();
}

/* =========================================================================
 * AMD-SPECIFIC ROTATION ROUTING PLANE (RAID 5 / RAID 6)
 * Calculates Left-Asymmetric Distributed Parity offsets across member slots.
 *
 * Vendor on-disk encoding (from rcraid.sys decompile FUN_140024884):
 *   RAID 5: FirstCount = N-1, SecondCount = 1, DeviceType = 0x1BFA (or 0x1BF6)
 *   RAID 6: FirstCount = N-2, SecondCount = 2, DeviceType = 0x1BF5 (or 0x1BF6)
 *   Chunk size ladder identical to RAID 0/10: chunk_index 1/2/3 → 64/128/256 KiB
 *   Layout: Left-asymmetric rotating parity (same as Linux md RAID-5/6)
 * ========================================================================= */

/**
 * rc_amd_map_distributed_raid5 - Map logical LBA to (target_member, parity_member, phys_lba)
 * @sector_lba:      logical sector address
 * @chunk_sectors:   stripe size in 512-byte sectors
 * @target_member:   out: data drive index (0..N-2)
 * @parity_member:   out: parity drive index for this stripe
 * @num_drives:      total drives in array (N)
 *
 * Left-asymmetric algorithm (matches Linux md, vendor decompile):
 *   - Parity rotates backward: parity = (N-1) - (stripe % N)
 *   - Data drives indexed 0..N-2, skipping parity position
 *   - Phys LBA = (stripe / (N-1)) * chunk_sectors + offset
 *
 * Returns: physical sector on target_member
 */
u64 rc_amd_map_distributed_raid5(u64 sector_lba, u32 chunk_sectors,
                                  int *target_member, int *parity_member, int num_drives)
{
    u64 stripe_num;
    u32 stripe_off;
    int data_drives = num_drives - 1;
    int parity_pos, data_idx;

    if (unlikely(num_drives < 3 || !chunk_sectors)) {
        *target_member = 0;
        *parity_member = 0;
        return sector_lba;
    }

    stripe_num  = div_u64(sector_lba, chunk_sectors);
    stripe_off  = (u32)(sector_lba - stripe_num * chunk_sectors);

    /* Parity rotates backward (left-asymmetric) */
    parity_pos = (num_drives - 1) - (int)(stripe_num % num_drives);
    *parity_member = parity_pos;

    /* Data index within the stripe (0..N-2) */
    data_idx = (int)(stripe_num % data_drives);

    /* Skip over parity position */
    if (data_idx >= parity_pos)
        data_idx++;
    *target_member = data_idx;

    /* Physical stripe within the data drive */
    return (div_u64(stripe_num, data_drives) * chunk_sectors) + stripe_off;
}
EXPORT_SYMBOL_GPL(rc_amd_map_distributed_raid5);

/**
 * rc_amd_map_distributed_raid6 - Map logical LBA to (target_member, p_member, q_member, phys_lba)
 * @sector_lba:      logical sector address
 * @chunk_sectors:   stripe size in 512-byte sectors
 * @target_member:   out: data drive index (0..N-3)
 * @p_member:        out: P parity drive index for this stripe
 * @q_member:        out: Q parity drive index for this stripe
 * @num_drives:      total drives in array (N)
 *
 * RAID 6 dual parity rotation (left-asymmetric, matches vendor uVar12=2):
 *   - Two parity drives rotate together: Q at (N-1) - (stripe % N)
 *     P at (N-2) - (stripe % N)  (wraps mod N)
 *   - Data drives indexed 0..N-3, skipping both parity positions
 *
 * Returns: physical sector on target_member
 */
u64 rc_amd_map_distributed_raid6(u64 sector_lba, u32 chunk_sectors,
                                  int *target_member, int *p_member, int *q_member,
                                  int num_drives)
{
    u64 stripe_num;
    u32 stripe_off;
    int data_drives = num_drives - 2;
    int q_pos, p_pos, data_idx;

    if (unlikely(num_drives < 4 || !chunk_sectors)) {
        *target_member = 0;
        *p_member = 0;
        *q_member = 0;
        return sector_lba;
    }

    stripe_num  = div_u64(sector_lba, chunk_sectors);
    stripe_off  = (u32)(sector_lba - stripe_num * chunk_sectors);

    /* Q parity rotates backward; P follows Q */
    q_pos = (num_drives - 1) - (int)(stripe_num % num_drives);
    p_pos = (num_drives - 2) - (int)(stripe_num % num_drives);
    if (p_pos < 0) p_pos += num_drives;

    *q_member = q_pos;
    *p_member = p_pos;

    /* Data index within the stripe (0..N-3) */
    data_idx = (int)(stripe_num % data_drives);

    /* Skip over both parity positions (P and Q) */
    if (data_idx >= p_pos) data_idx++;
    if (data_idx >= q_pos) data_idx++;
    *target_member = data_idx;

    /* Physical stripe within the data drive */
    return (div_u64(stripe_num, data_drives) * chunk_sectors) + stripe_off;
}
EXPORT_SYMBOL_GPL(rc_amd_map_distributed_raid6);

/* Thin routing wrappers for callers that use the old single-output API */
void rc_amd_route_io_distributed_raid5(u64 *lba, int *mbr, int *parity_mbr,
                                        u32 chunk_sectors, int num_drives)
{
    *lba = rc_amd_map_distributed_raid5(*lba, chunk_sectors, mbr, parity_mbr, num_drives);
}
EXPORT_SYMBOL_GPL(rc_amd_route_io_distributed_raid5);

void rc_amd_route_io_distributed_raid6(u64 *lba, int *mbr, int *p_mbr, int *q_mbr,
                                        u32 chunk_sectors, int num_drives)
{
    *lba = rc_amd_map_distributed_raid6(*lba, chunk_sectors, mbr, p_mbr, q_mbr, num_drives);
}
EXPORT_SYMBOL_GPL(rc_amd_route_io_distributed_raid6);