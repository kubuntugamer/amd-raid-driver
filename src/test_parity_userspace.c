/**
 * ==============================================================================
 * USERSpace TEST HARNESS: RAID 5/6 Parity Math Verification
 * Compiles with: gcc -O2 -std=c11 -Wall -Wextra -o test_parity test_parity_userspace.c
 * ==============================================================================
 *
 * Verifies:
 *   1. GF(2^8) tables (exp/log/mul) match mathematical reference
 *   2. RAID 5 P parity = XOR across data drives
 *   3. RAID 6 Q parity = Reed-Solomon Σ(di × α^i)
 *   4. Distributed parity mapping (RAID 5/6) for N=3..8
 *   5. Round-trip: generate → corrupt one/two drives → reconstruct → verify
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* =========================================================================
 * GF(2^8) ARITHMETIC — copied from patch_parity_math.c
 * Primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1 (0x11D)
 * Generator: α = 0x02
 * ========================================================================= */

#define GF28_POLY  0x11D
#define GF28_ALPHA 0x02

static uint8_t gf28_exp[256];
static uint8_t gf28_log[256];
static uint8_t gf28_mul[256];

static void gf28_init_tables(void)
{
    int i;
    uint16_t v = 1;

    for (i = 0; i < 255; i++) {
        gf28_exp[i] = (uint8_t)v;
        gf28_log[v] = (uint8_t)i;
        v <<= 1;
        if (v & 0x100)
            v ^= GF28_POLY;
    }
    gf28_exp[255] = gf28_exp[0];
    gf28_log[0] = 255;

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

static inline uint8_t gf28_mul_general(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0)
        return 0;
    return gf28_exp[(gf28_log[a] + gf28_log[b]) % 255];
}

/* =========================================================================
 * PARITY GENERATION — copied from patch_parity_math.c
 * ========================================================================= */

void rc_amd_generate_raid5_parity(uint8_t **data_buffers, uint8_t *parity_buffer,
                                   size_t block_len, int num_drives)
{
    int data_drives = num_drives - 1;
    size_t i;
    int d;

    if (data_drives <= 0 || block_len == 0)
        return;

    memset(parity_buffer, 0, block_len);

    for (d = 0; d < data_drives; d++) {
        uint8_t *src = data_buffers[d];
        for (i = 0; i < block_len; i++)
            parity_buffer[i] ^= src[i];
    }
}

void rc_amd_generate_raid6_parity(uint8_t **data_buffers, uint8_t *p_buffer,
                                   uint8_t *q_buffer, size_t block_len, int num_drives)
{
    int data_drives = num_drives - 2;
    size_t i;
    int d;

    if (data_drives <= 0 || block_len == 0)
        return;

    memset(p_buffer, 0, block_len);
    memset(q_buffer, 0, block_len);

    for (i = 0; i < block_len; i++) {
        uint8_t p = 0;
        uint8_t q = 0;

        for (d = 0; d < data_drives; d++) {
            uint8_t byte = data_buffers[d][i];
            p ^= byte;
            if (byte != 0) {
                int exp_idx = d % 255;
                uint8_t coeff = gf28_exp[exp_idx];
                q ^= gf28_mul_general(byte, coeff);
            }
        }

        p_buffer[i] = p;
        q_buffer[i] = q;
    }
}

/* =========================================================================
 * DISTRIBUTED PARITY MAPPING — copied from patch_parity_math.c
 * ========================================================================= */

uint64_t rc_amd_map_distributed_raid5(uint64_t sector_lba, uint32_t chunk_sectors,
                                       int *target_member, int *parity_member, int num_drives)
{
    uint64_t stripe_num;
    uint32_t stripe_off;
    int data_drives = num_drives - 1;
    int parity_pos, data_idx;

    if (num_drives < 3 || !chunk_sectors) {
        *target_member = 0;
        *parity_member = 0;
        return sector_lba;
    }

    stripe_num  = sector_lba / chunk_sectors;
    stripe_off  = (uint32_t)(sector_lba - stripe_num * chunk_sectors);

    parity_pos = (num_drives - 1) - (int)(stripe_num % num_drives);
    *parity_member = parity_pos;

    data_idx = (int)(stripe_num % data_drives);
    if (data_idx >= parity_pos)
        data_idx++;
    *target_member = data_idx;

    return (stripe_num * chunk_sectors) + stripe_off;
}

uint64_t rc_amd_map_distributed_raid6(uint64_t sector_lba, uint32_t chunk_sectors,
                                       int *target_member, int *p_member, int *q_member,
                                       int num_drives)
{
    uint64_t stripe_num;
    uint32_t stripe_off;
    int data_drives = num_drives - 2;
    int q_pos, p_pos, data_idx;

    if (num_drives < 4 || !chunk_sectors) {
        *target_member = 0;
        *p_member = 0;
        *q_member = 0;
        return sector_lba;
    }

    stripe_num  = sector_lba / chunk_sectors;
    stripe_off  = (uint32_t)(sector_lba - stripe_num * chunk_sectors);

    q_pos = (num_drives - 1) - (int)(stripe_num % num_drives);
    p_pos = (num_drives - 2) - (int)(stripe_num % num_drives);
    if (p_pos < 0) p_pos += num_drives;

    *q_member = q_pos;
    *p_member = p_pos;

    data_idx = (int)(stripe_num % data_drives);
    if (data_idx >= p_pos) data_idx++;
    if (data_idx >= q_pos) data_idx++;
    *target_member = data_idx;

    return (stripe_num * chunk_sectors) + stripe_off;
}

/* =========================================================================
 * TEST VECTORS & VERIFICATION
 * ========================================================================= */

#define TEST_BLOCK_LEN  4096

static void test_gf28_tables(void)
{
    printf("Testing GF(2^8) tables...\n");
    
    /* Verify exp/log are inverses */
    for (int i = 1; i < 256; i++) {
        uint8_t log_val = gf28_log[i];
        uint8_t exp_val = gf28_exp[log_val];
        assert(exp_val == i);
    }
    
    /* Verify α^255 = 1 */
    assert(gf28_exp[255] == 1);
    assert(gf28_exp[0] == 1);
    
    /* Verify α^i * α^j = α^(i+j mod 255) */
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            uint8_t a = gf28_exp[i];
            uint8_t b = gf28_exp[j];
            uint8_t prod = gf28_mul_general(a, b);
            uint8_t expected = gf28_exp[(i + j) % 255];
            assert(prod == expected);
        }
    }
    
    /* Verify multiplication by α table */
    for (int i = 1; i < 256; i++) {
        uint8_t expected = gf28_mul_general(i, 2);
        assert(gf28_mul[i] == expected);
    }
    
    printf("  GF(2^8) tables: PASS\n");
}

static void test_raid5_parity(void)
{
    printf("Testing RAID 5 parity...\n");
    
    const int num_drives = 5;  /* 4 data + 1 parity */
    const int data_drives = num_drives - 1;
    
    uint8_t *buffers[4];
    uint8_t parity[TEST_BLOCK_LEN];
    
    for (int d = 0; d < data_drives; d++) {
        buffers[d] = malloc(TEST_BLOCK_LEN);
        /* Fill with known pattern */
        for (int i = 0; i < TEST_BLOCK_LEN; i++)
            buffers[d][i] = (uint8_t)(d * 73 + i * 17);
    }
    
    rc_amd_generate_raid5_parity(buffers, parity, TEST_BLOCK_LEN, num_drives);
    
    /* Verify P = XOR of all data */
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        uint8_t expected = 0;
        for (int d = 0; d < data_drives; d++)
            expected ^= buffers[d][i];
        assert(parity[i] == expected);
    }
    
    /* Test reconstruction: corrupt one drive, rebuild from parity + others */
    uint8_t *corrupted = malloc(TEST_BLOCK_LEN);
    memcpy(corrupted, buffers[2], TEST_BLOCK_LEN);
    
    /* Zero out drive 2 to simulate failure */
    memset(buffers[2], 0, TEST_BLOCK_LEN);
    
    /* Rebuild drive 2: parity ⊕ all other drives */
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        uint8_t rebuilt = parity[i];
        for (int d = 0; d < data_drives; d++) {
            if (d != 2)
                rebuilt ^= buffers[d][i];
        }
        assert(rebuilt == corrupted[i]);
    }
    
    for (int d = 0; d < data_drives; d++) free(buffers[d]);
    free(corrupted);
    
    printf("  RAID 5 parity: PASS\n");
}

static void test_raid6_parity(void)
{
    printf("Testing RAID 6 parity...\n");
    
    const int num_drives = 6;  /* 4 data + 2 parity (P+Q) */
    const int data_drives = num_drives - 2;
    
    uint8_t *buffers[4];
    uint8_t p_parity[TEST_BLOCK_LEN];
    uint8_t q_parity[TEST_BLOCK_LEN];
    
    for (int d = 0; d < data_drives; d++) {
        buffers[d] = malloc(TEST_BLOCK_LEN);
        for (int i = 0; i < TEST_BLOCK_LEN; i++)
            buffers[d][i] = (uint8_t)(d * 97 + i * 31);
    }
    
    rc_amd_generate_raid6_parity(buffers, p_parity, q_parity, TEST_BLOCK_LEN, num_drives);
    
    /* Verify P = XOR */
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        uint8_t expected = 0;
        for (int d = 0; d < data_drives; d++)
            expected ^= buffers[d][i];
        assert(p_parity[i] == expected);
    }
    
    /* Verify Q = Σ(di × α^i) */
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        uint8_t expected = 0;
        for (int d = 0; d < data_drives; d++) {
            uint8_t byte = buffers[d][i];
            if (byte != 0) {
                int exp_idx = d % 255;
                uint8_t coeff = gf28_exp[exp_idx];
                expected ^= gf28_mul_general(byte, coeff);
            }
        }
        assert(q_parity[i] == expected);
    }
    
    /* Test reconstruction: lose ONE drive, rebuild from P + remaining */
    uint8_t *lost_drive = malloc(TEST_BLOCK_LEN);
    memcpy(lost_drive, buffers[1], TEST_BLOCK_LEN);
    memset(buffers[1], 0, TEST_BLOCK_LEN);
    
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        uint8_t rebuilt = p_parity[i];
        for (int d = 0; d < data_drives; d++) {
            if (d != 1)
                rebuilt ^= buffers[d][i];
        }
        assert(rebuilt == lost_drive[i]);
    }
    
    memcpy(buffers[1], lost_drive, TEST_BLOCK_LEN);
    free(lost_drive);
    
    /* Test reconstruction: lose TWO drives, rebuild from P+Q + remaining */
    uint8_t *lost1 = malloc(TEST_BLOCK_LEN);
    uint8_t *lost2 = malloc(TEST_BLOCK_LEN);
    memcpy(lost1, buffers[0], TEST_BLOCK_LEN);
    memcpy(lost2, buffers[3], TEST_BLOCK_LEN);
    memset(buffers[0], 0, TEST_BLOCK_LEN);
    memset(buffers[3], 0, TEST_BLOCK_LEN);
    
    for (int i = 0; i < TEST_BLOCK_LEN; i++) {
        /* P' = P ⊕ d1 ⊕ d2 = d0 ⊕ d3 */
        uint8_t p_prime = p_parity[i] ^ buffers[1][i] ^ buffers[2][i];
        
        /* Q' = Q ⊕ d1×α¹ ⊕ d2×α² = d0 ⊕ d3×α³ */
        uint8_t q_prime = q_parity[i];
        if (buffers[1][i] != 0)
            q_prime ^= gf28_mul_general(buffers[1][i], gf28_exp[1]);
        if (buffers[2][i] != 0)
            q_prime ^= gf28_mul_general(buffers[2][i], gf28_exp[2]);
        
        /* d0 ⊕ d3 = P'
         * d0 ⊕ d3×α³ = Q'
         * d3 = (P' ⊕ Q') / (1 ⊕ α³)
         */
        uint8_t alpha3 = gf28_exp[3];
        uint8_t denom = 1 ^ alpha3;
        uint8_t numerator = p_prime ^ q_prime;
        uint8_t denom_inv = gf28_exp[(255 - gf28_log[denom]) % 255];
        uint8_t d3 = gf28_mul_general(numerator, denom_inv);
        uint8_t d0 = p_prime ^ d3;
        
        assert(d0 == lost1[i]);
        assert(d3 == lost2[i]);
    }
    
    for (int d = 0; d < data_drives; d++) free(buffers[d]);
    free(lost1);
    free(lost2);
    
    printf("  RAID 6 parity: PASS\n");
}

static void test_mapping_functions(void)
{
    printf("Testing distributed parity mapping...\n");
    
    const uint32_t chunk_sectors = 512;  /* 256 KiB stripe */
    
    /* Test RAID 5 mapping for various N */
    for (int n = 3; n <= 8; n++) {
        for (uint64_t lba = 0; lba < 10000; lba += 17) {
            int target, parity;
            uint64_t phys = rc_amd_map_distributed_raid5(lba, chunk_sectors, &target, &parity, n);
            
            assert(target >= 0 && target < n);
            assert(parity >= 0 && parity < n);
            assert(target != parity);
            assert(phys < UINT64_MAX);
        }
        
        /* Verify parity rotation: each stripe should have different parity */
        int parity_counts[8] = {0};
        for (int stripe = 0; stripe < n * 100; stripe++) {
            int t, p;
            uint64_t s = (uint64_t)stripe * chunk_sectors;
            rc_amd_map_distributed_raid5(s, chunk_sectors, &t, &p, n);
            parity_counts[p]++;
        }
        for (int i = 0; i < n; i++)
            assert(parity_counts[i] > 0);  /* Each drive should be parity at least once */
    }
    
    /* Test RAID 6 mapping for various N */
    for (int n = 4; n <= 8; n++) {
        for (uint64_t lba = 0; lba < 10000; lba += 23) {
            int target, p_mem, q_mem;
            rc_amd_map_distributed_raid6(lba, chunk_sectors, &target, &p_mem, &q_mem, n);
            
            assert(target >= 0 && target < n);
            assert(p_mem >= 0 && p_mem < n);
            assert(q_mem >= 0 && q_mem < n);
            assert(target != p_mem);
            assert(target != q_mem);
            assert(p_mem != q_mem);
        }
        
        /* Verify both parities rotate */
        int p_counts[8] = {0}, q_counts[8] = {0};
        for (int stripe = 0; stripe < n * 100; stripe++) {
            int t, p, q;
            uint64_t s = (uint64_t)stripe * chunk_sectors;
            rc_amd_map_distributed_raid6(s, chunk_sectors, &t, &p, &q, n);
            p_counts[p]++;
            q_counts[q]++;
        }
        for (int i = 0; i < n; i++) {
            assert(p_counts[i] > 0);
            assert(q_counts[i] > 0);
        }
    }
    
    printf("  Mapping functions: PASS\n");
}

static void test_known_vectors(void)
{
    printf("Testing known GF(2^8) vectors...\n");
    
    /* Known test: α^0 = 1, α^1 = 2, α^2 = 4, α^3 = 8, α^4 = 16, α^5 = 32, α^6 = 64, α^7 = 128 */
    assert(gf28_exp[0] == 0x01);
    assert(gf28_exp[1] == 0x02);
    assert(gf28_exp[2] == 0x04);
    assert(gf28_exp[3] == 0x08);
    assert(gf28_exp[4] == 0x10);
    assert(gf28_exp[5] == 0x20);
    assert(gf28_exp[6] == 0x40);
    assert(gf28_exp[7] == 0x80);
    
    /* α^8 = 0x1D (x^8 + x^4 + x^3 + x^2 + 1 = 0x11D → x^8 = x^4+x^3+x^2+1 = 0x1D) */
    assert(gf28_exp[8] == 0x1D);
    
    /* 0x57 × 0x83 = 0x31 (verified for 0x11D polynomial, α=0x02) */
    uint8_t test = gf28_mul_general(0x57, 0x83);
    assert(test == 0x31);
    
    printf("  Known vectors: PASS\n");
}

static void test_raid6_reconstruction_edge_cases(void)
{
    printf("Testing RAID 6 reconstruction edge cases...\n");
    
    const int num_drives = 8;  /* Max: 6 data + 2 parity */
    const int data_drives = num_drives - 2;
    const size_t len = 1024;
    
    /* Use fixed seed for reproducibility */
    srand(42);
    
    uint8_t *buffers[6];
    uint8_t p_parity[len];
    uint8_t q_parity[len];
    
    for (int d = 0; d < data_drives; d++) {
        buffers[d] = malloc(len);
        for (size_t i = 0; i < len; i++)
            buffers[d][i] = (uint8_t)rand();
    }
    
    rc_amd_generate_raid6_parity(buffers, p_parity, q_parity, len, num_drives);
    
    /* Test all pairs of drive failures */
    for (int fail1 = 0; fail1 < data_drives; fail1++) {
        for (int fail2 = fail1 + 1; fail2 < data_drives; fail2++) {
            /* Save lost data */
            uint8_t *lost1 = malloc(len);
            uint8_t *lost2 = malloc(len);
            memcpy(lost1, buffers[fail1], len);
            memcpy(lost2, buffers[fail2], len);
            
            /* Zero them */
            memset(buffers[fail1], 0, len);
            memset(buffers[fail2], 0, len);
            
            /* Rebuild using P+Q */
            for (size_t i = 0; i < len; i++) {
                /* Compute P' = P ⊕ (all surviving data) = d_f1 ⊕ d_f2 */
                uint8_t p_prime = p_parity[i];
                for (int d = 0; d < data_drives; d++) {
                    if (d != fail1 && d != fail2)
                        p_prime ^= buffers[d][i];
                }
                
                /* Compute Q' = Q ⊕ (all surviving × α^d) = d_f1×α^f1 ⊕ d_f2×α^f2 */
                uint8_t q_prime = q_parity[i];
                for (int d = 0; d < data_drives; d++) {
                    if (d != fail1 && d != fail2) {
                        int exp_idx = d % 255;
                        uint8_t coeff = gf28_exp[exp_idx];
                        /* Always XOR out the contribution (0 * coeff = 0 is fine) */
                        q_prime ^= gf28_mul_general(buffers[d][i], coeff);
                    }
                }
                
                uint8_t alpha_f1 = gf28_exp[fail1 % 255];
                uint8_t alpha_f2 = gf28_exp[fail2 % 255];
                
                uint8_t p_times_af1 = gf28_mul_general(p_prime, alpha_f1);
                uint8_t numerator = q_prime ^ p_times_af1;
                uint8_t denom = alpha_f1 ^ alpha_f2;
                
                uint8_t denom_inv = gf28_exp[(255 - gf28_log[denom]) % 255];
                
                uint8_t d_fail2 = gf28_mul_general(numerator, denom_inv);
                uint8_t d_fail1 = p_prime ^ d_fail2;
                
                assert(d_fail1 == lost1[i]);
                assert(d_fail2 == lost2[i]);
            }
            
            /* Restore */
            memcpy(buffers[fail1], lost1, len);
            memcpy(buffers[fail2], lost2, len);
            free(lost1);
            free(lost2);
        }
    }
    
    for (int d = 0; d < data_drives; d++) free(buffers[d]);
    
    printf("  RAID 6 all-pair reconstruction: PASS\n");
}

/* A distributed layout must be one-to-one: no two logical sectors may map to
 * the same (member, physical LBA), and the P/Q/data roles of a stripe must be
 * distinct members. */
static int check_bijection(int n, int is_raid6)
{
    const uint32_t chunk_sectors = 8;
    const uint64_t lbas = 4096;
    uint8_t *seen = calloc((size_t)n * lbas, 1);
    int ok = 1;

    assert(seen);
    for (uint64_t lba = 0; lba < lbas && ok; lba++) {
        int t, p, q;
        uint64_t phys;
        if (is_raid6) {
            phys = rc_amd_map_distributed_raid6(lba, chunk_sectors, &t, &p, &q, n);
            if (t == p || t == q || p == q)
                ok = 0;
        } else {
            phys = rc_amd_map_distributed_raid5(lba, chunk_sectors, &t, &p, n);
            if (t == p)
                ok = 0;
        }
        if (phys >= lbas || seen[(size_t)t * lbas + phys]++)
            ok = 0;
        if (!ok)
            printf("  RAID%d n=%d: lba %lu collides or clashes\n",
                   is_raid6 ? 6 : 5, n, (unsigned long)lba);
    }
    free(seen);
    return ok;
}

static void test_mapping_bijection(void)
{
    printf("Testing distributed mapping is one-to-one...\n");
    for (int n = 3; n <= 8; n++)
        assert(check_bijection(n, 0));
    for (int n = 4; n <= 8; n++)
        assert(check_bijection(n, 1));
    printf("  Mapping bijection: PASS\n");
}

int main(void)
{
    printf("========================================\n");
    printf("RAID 5/6 Parity Math Test Harness\n");
    printf("========================================\n\n");
    
    gf28_init_tables();
    
    test_known_vectors();
    test_gf28_tables();
    test_raid5_parity();
    test_raid6_parity();
    test_mapping_functions();
    test_mapping_bijection();
    test_raid6_reconstruction_edge_cases();
    
    printf("\n========================================\n");
    printf("ALL TESTS PASSED ✓\n");
    printf("========================================\n");
    
    return 0;
}