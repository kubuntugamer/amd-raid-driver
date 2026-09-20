# 🧵 FabricZC AI Execution & Bug-Auditing Blueprint Manual

## 📊 Subsystem Progress & Current Version State
* **Current Workspace Version:** 0.1.5
* **Staged Branch Context:** experimental
* **Phase 1-4:** COMPLETE
* **Phase 5 (Block MQ Pass-Through Pipeline):** ACTIVE / PROPROMPT READY

---

## 🔩 Low-Level Core Container Specification
All member block devices contain a 48-byte packed structure (`struct fabriczc_container_header`) written at the 16 KiB front-offset anchor zone, tracking sequence IDs, Fletcher64 checksums, container state magic, UUIDs, disk slot boundaries, and extent chunk sectors.

---

## 🤖 AI INGESTION SPECIFICATIONS FOR SOURCE CODE BUG AUDITING
Automated LLMs or AI generation engines ingesting this workspace must evaluate source files (such as `src/kernel/main.c` against `staging_includes/fabriczc_staging.h`) for block multi-queue safety, cache coherency, and thread isolation across concurrent hardware queues.

---

## 🧪 MANUAL SIMULATION RUNTIME CHECKS (VBOX LIVE USB LAB)
Run the automated installer and insert the module:
```bash
./setup_and_build.sh
sudo insmod fabriczc_mod.ko
```
Verify partitioning natively via `parted` and `lsblk` to ensure the multi-queue routing pipeline is operational.
