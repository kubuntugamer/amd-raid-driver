# 🤖 FabricZC AI System Specification Sheet

```text
[SYSTEM INTERACTION MATRIX]
┌────────────────────────────────────────────────────────┐
│             FabricZC Unified Kernel Subsystem          │
├───────────────────────────┬────────────────────────────┤
│   Asynchronous Layer      │   Zero-Copy Fabric Layer   │
│   • io_uring loops        │   • P2PDMA Bypass Engine   │
│   • Per-thread sharding   │   • Direct GPU VRAM Streams  │
└───────────────────────────┴────────────────────────────┘
```

## 🔩 1. Core Header Definition (Disk Layout Constraints)
All member block devices must contain a 48-byte packed structure written exactly at the **16 KiB front-offset anchor zone** and duplicated at the terminal mirror sector bounds. No other metadata signatures are permitted.

```c
struct fabriczc_container_header {
    uint64_t sequence_generation_id;  /* Offset 0x00: Monotonically increasing counter */
    uint64_t fletcher64_checksum;     /* Offset 0x08: Checked over offsets 0x10 to 0x30 */
    uint32_t container_state_magic;   /* Offset 0x10: 0x48435a21 ("!ZCH") or 0x44435a21 ("!ZCD") */
    uint8_t  container_uuid[16];      /* Offset 0x14: Global unique array passport token */
    uint8_t  active_member_slot;      /* Offset 0x24: Topology index bounds [0-7] */
    uint8_t  total_active_disks;      /* Offset 0x25: Current active array drive scale-out count */
    uint8_t  max_disk_boundary;       /* Offset 0x26: Enforced safety capacity limit hardcoded to 8 */
    uint8_t  reserved_padding;        /* Offset 0x27: Preserves 32-bit internal word alignments */
    uint32_t extent_chunk_sectors;    /* Offset 0x28: Sector chunk depth locked to 2048 (1 MiB) */
} __attribute__((packed));            /* Strict evaluation size requirement: Exactly 48 Bytes */
```

## 🎛️ 2. Core Execution State Routines
When writing code for the driver implementation loops, enforce these structural bounds:

### Validation Rules
*   **Checksum Verification:** Compute using the parallel Fletcher-64 algorithm. Read blocks from offset `0x10` through `0x30`. The computation must execute in \(\le 80\) nanoseconds.
*   **Magic State Machine:**
    *   `0x48435a21` (`!ZCH`): Array is healthy. Mount immediately without consistency sweeping.
    *   `0x44435a21` (`!ZCD`): Array is dirty (unclean unmount). Stop initialization. Replay active transaction ledger logs before presenting the block node.

### Topology Mapping Rules
*   **Stripe Chunk Size:** Hardcoded strictly to `2048` sectors (1 MiB block frames). 
*   **Scale-Out Bounds:** Minimum 1 drive, maximum 8 drives.
*   **Striping Geometry:** Horizontal RAID 0 chunk mapping. Sector addresses translate via the standard interleaving formula:
    \[	ext{Target Device Index} = \left(rac{	ext{Logical Sector}}{	ext{extent\_chunk\_sectors}}ight) \pmod{	ext{total\_active\_disks}}\]

## 🏎️ 3. Asynchronous I/O & Memory Routing Bounds
*   **Control Path:** Driven natively via kernel-space `io_uring` ring submission and completion queues. Traditional synchronous read/write entry points are forbidden.
*   **Parallelization:** Divide memory spaces into independent **Allocation Groups**. Each active CPU thread owns a single allocation group lock context, eliminating core cross-locking contentions.
*   **Data Path (Zero-Copy):** Integrate kernel **P2PDMA** page mapping routines. When processing asset requests, verify page structures. If the destination memory address belongs to a registered PCIe GPU memory map, map the NVMe controller queues straight to the target VRAM address space, bypassing host system RAM bounce-buffers completely.

## 🔀 4. Online Capacity Expansion (OCE) Mechanics
When an expansion instruction is signaled to add new drives up to the limit of 8:
1.  **Freeze Allocations:** Temporarily pause active `io_uring` queue execution rings.
2.  **Zero Metadata Margins:** Issue a sequential `dd` wipe pass over the 16 KiB front-offsets and tail sectors of the incoming new drive nodes.
3.  **Update Headers:** Increment the `total_active_disks` tracking value inside the header structure. Recalculate the Fletcher-64 checksum and flash the updated layout back across all active container devices concurrently.
4.  **Asynchronous Allocation Transition:** Unfreeze queues. Keep existing data blocks static on historical sectors. Route all fresh 1 MiB write assignments over the expanded striped layout immediately.
5.  **Idle Rebalance Pass:** Activate a low-priority background migration loop only when storage bus activity drops to zero. Copy single-track historical allocations and distribute them into aligned horizontal stripes across the expanded array channels.