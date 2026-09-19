# 🧵 FabricZC AI Execution & Progression Blueprint Manual

## 📊 Subsystem Progress & Current Version State
* **Current Workspace Version:** 0.1.0
* **Staged Branch Context:** experimental
* **Phase 1 (Skeleton Bootstrap):** COMPLETE
* **Phase 2 (Asynchronous Ring Mappings):** COMPLETE
* **Phase 3 (P2PDMA Engine Hooking):** COMPLETE
* **Phase 4 (SGL Translation Matrix):** PENDING / ACTIVE NEXT TASK

---

## 🔩 Low-Level Core Container Specification
All member block devices contain a 48-byte packed structure written at the **16 KiB front-offset anchor zone**:
```c
struct fabriczc_container_header {
    uint64_t sequence_generation_id;
    uint64_t fletcher64_checksum;
    uint32_t container_state_magic;
    uint8_t  container_uuid[16];
    uint8_t  active_member_slot;
    uint8_t  total_active_disks;
    uint8_t  max_disk_boundary;
    uint8_t  reserved_padding;
    uint32_t extent_chunk_sectors;
} __attribute__((packed));
```

---

## 🤖 AI CORE INGESTION SPECIFICATIONS FOR PHASE 4 PROGRESSION
When an automated LLM or AI generation engine ingests this workspace tree to execute the next phase, it must strictly interpret and execute these instructions:

### 1. Ingestion Requirements
* **Context Ingestion:** Load `staging_includes/fabriczc_staging.h` and evaluate the existing `struct fabriczc_p2p_engine` arrays.
* **Target File Path:** Mutate and expand the function loops directly inside `src/kernel/main.c`.
* **Compilation Rule:** The generated code must compile flawlessly using the freestanding sandbox compiler tool (`./run_pure_local_compile.sh`) under `-std=gnu11` with zero core system header dependencies.

### 2. Functional Code Generation Prompt
"Implement **Phase 4: Scatter-Gather List (SGL) Descriptor Translation**. You must generate a lock-free, freestanding `fabriczc_translate_sgl_to_p2p` function loop inside `src/kernel/main.c`. 
The function signature must accept a memory address descriptor vector, translate logical blocks using the horizontal RAID 0 interleaved math (`Target Device Index = (Logical Sector / 2048) % total_active_disks`), map the segment straight to the validated `pci_p2pdma` VRAM coordinates, and return an explicit validation code tracking status metric. Ensure absolute compliance with cache-line alignment variables and lock-free thread isolation bounds."

### 3. Verification Script Generation Task
"Following successful compilation of the `fabriczc_mod.ko` object binary, autonomously output a localized user-space verification script named `verify_sgl_matrix.sh` inside the sandbox folder to test memory mapping registers via fake software transaction streams."

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.

---

## 🧪 MANUAL SIMULATION RUNTIME CHECKS (VBOX LIVE USB LAB)

When you boot your Kubuntu Live USB inside VirtualBox with your 4 blank NVMe disks attached, open a terminal window and run these validation commands manually:

### 1. Rebuild and Mount the Driver Module
Pull your standalone repository code inside the volatile RAM space, compile the source targets, and insert the block layer driver:
\`\`\`bash
# Run the local standalone compile loop wrapper
./run_pure_local_compile.sh

# Inject the compiled hybrid block engine into active memory
sudo insmod fabriczc_mod.ko
\`\`\`

### 2. Verify the Subsystem Takeover & XFS Spoofing Node
Check the kernel log buffers immediately to confirm your parallel Allocation Group channels initialized and the XFSB magic overrides are armed:
\`\`\`bash
# Output the trailing kernel buffer logs
sudo dmesg | tail -n 20

# Confirm your unified, single aggregate device block node exists
ls -lh /dev/rcraid0
\`\`s

### 3. Run the Installer Validation Test Pass
Simulate an installer query tool scanning the drive to verify that your custom memory-mapped intercepts successfully trick the software into seeing a pre-formatted XFS partition:
\`\`\`bash
# Force a block device identification scan on the hybrid device
sudo blkid /dev/rcraid0
\`\`\`
*Expected Diagnostic Output:* The scan should bypass the blank sectors and return: \`/dev/rcraid0: TYPE="xfs"\`, verifying your software-defined controller works flawlessly!

---

## 🤖 PHASE 5 AUTOMATED PROGRESSION TARGETS
* **Active Milestone:** Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping)
* **AI Ingestion Rule:** Load 'src/kernel/main.c', implement lock-free extent linear logic mapping calculations, and pass compilation using the standalone './run_pure_local_compile.sh' wrapper flags.
