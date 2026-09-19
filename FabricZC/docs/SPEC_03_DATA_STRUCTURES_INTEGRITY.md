# 🔩 SPEC 03: Data Structures & State-Aware Integrity Matrix

## 1. Binary Footprint & Layout Anchoring
The container layout enforces a strict binary disk configuration footprint. Every member block device maps its metadata directly onto two isolated anchoring zones.
* **Primary Anchor:** Stamped squarely inside the 16 KiB front-offset zone (bypassing master boot sector tracks).
* **Terminal Mirror:** Stamped squarely at the absolute final sector boundary block of the physical drive capacity.

## 2. The 48-Byte Packed Management Struct
```c
struct fabriczc_container_header {
    uint64_t sequence_generation_id;  /* Monotonically increasing ledger number */
    uint64_t fletcher64_checksum;     /* Validation token over offsets 0x10 to 0x30 */
    uint32_t container_state_magic;   /* 0x48435a21 ("!ZCH") or 0x44435a21 ("!ZCD") */
    uint8_t  container_uuid[16];      /* Global unique array passport identity hash */
    uint8_t  active_member_slot;      /* Topology index tracker [0 through 7] */
    uint8_t  total_active_disks;      /* Dynamic active drive scale-out count indicator */
    uint8_t  max_disk_boundary;       /* Physical safety ceiling hardcoded to 8 */
    uint8_t  reserved_padding;        /* Alignment pad to maintain 32-bit internal words */
    uint32_t extent_chunk_sectors;    /* Sector block chunk depth locked to 2048 (1 MiB) */
} __attribute__((packed));
```

## 3. 80-Nanosecond Fletcher-64 Verification Loop
Data integrity status states are calculated utilizing a parallelized Fletcher-64 block arithmetic loop:
```c
uint64_t calculate_fletcher64(const uint32_t *data, size_t words) {
    uint32_t sum1 = 0, sum2 = 0;
    for (size_t i = 0; i < words; ++i) {
        sum1 += data[i];
        sum2 += sum1;
    }
    return ((uint64_t)sum2 << 32) | sum1;
}
```
* **Immediate Health Decisions:** If container_state_magic matches 0x48435a21 (!ZCH), initialization completes instantly. If it matches 0x44435a21 (!ZCD), the engine drops into immediate log-replay passes to clean up active 1 MiB extent transactions before mounting.