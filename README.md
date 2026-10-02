# ⚡ rcraid (Multi-Format TUI-Enhanced Edition)

[![License: GPL-2.0-only](https://img.shields.io/badge/License-GPL--2.0--only-blue.svg)](https://opensource.org/license/gpl-2-0)
![Kernel: Linux 7.0+](https://img.shields.io/badge/Kernel-Linux%207.0%2B-orange.svg)
![DKMS: out-of-tree](https://img.shields.io/badge/DKMS-out--of--tree-informational.svg)

Your motherboard's AMD RAIDXpert2 NVMe storage arrays become a native, standard block device node right at **/dev/rcraid0**. Partition it, format it, configure it dynamically from a clean terminal wizard, and **boot Linux straight from it with zero graphical overhead.**

---

## 🚦 Deployment Status — Baseline Release (2026-10-02)

> **⚠️ DEPLOYMENT WARNING — READ BEFORE INSTALLING.**
> The committed baseline is **RAID 0, RAID 1, and RAID 10**; **RAID 10 is now complete
> end-to-end and stable for production flash deployments** (reads/writes/discards, degraded failover,
> member rebuild, and boot-time degraded assembly all landed and validated).
> The mirror completion and array rebuild paths run on a
> **modern non-blocking asynchronous thread runner** (`kamd_async_worker` parks
> parent-bio endio off the hardware completion context; `kamd_resync` owns
> long-running rebuilds), so hardware completion contexts never block. All
> **legacy mechanical spinning-platter (SATA HDD) models are completely
> dropped** — this branch services the AMD NVMe RAID controller (`1022:B000`)
> only. Do not deploy it against a spinning-disk array.

*   **Kernel target:** Linux **7.0+**, consumed as an **out-of-tree DKMS**
    module built against the running kernel's headers — not an in-tree driver.
*   **Verified build:** the tree compiles **cleanly, with zero warnings and zero
    errors**, against `7.0.0-34-generic` headers (a second clean build was
    verified against `7.0.0-38-generic`).
*   **RAID 10 (Nested Striped Mirror):** the nested striped-mirror layout takes a real blk-mq
    dispatch path — writes and discards mirror to **both** copies of each
    2-way pair, reads round-robin across the pair, and losing one copy per
    pair degrades the array instead of failing it. A stale or replaced member
    is rebuilt **from its own mirror partner** (the only copy that holds the
    same column's stripes), one column at a time, with a logical
    write-exclusion window so application writes can't race the copy. The
    array **assembles degraded at boot** as long as every pair keeps at least
    one copy, then rebuilds the missing copy when it returns; a pair that has
    lost both copies refuses assembly rather than exposing an unreadable
    volume.
*   **RAID 10 geometry fix (2026-10-02):** the `rc_amd_map_nested_raid10` translation plane now correctly splits addresses across flash geometries with no single-stripe bottlenecks. Per-column physical LBA calculation uses `div_u64` for 64-bit safety; column assignment via modulo arithmetic is branch-predictor friendly. Legacy block boundaries restricting enterprise flash throughput have been removed.
*   **Fault isolation hardened:** all transient device fault-isolation loops now cleanly catch and handle `BLK_STS_RESOURCE` boundaries, guaranteeing total array resilience against unexpected hardware dropouts. The async completion engine returns `BLK_STS_RESOURCE` on allocation failure or unload race, never `BLK_STS_IOERR`, so the block layer retries instead of reporting false I/O errors to the filesystem.
*   **Frozen scope:** features outside the RAID 0 / RAID 1 / RAID 10 data
    paths — the RAID 5 / RAID 6 layouts and the remaining experimental engine
    code paths — are **intentionally frozen** for **post-migration execution
    phases**. They remain in-tree but are experimental; do not rely on them for
    production data.

---

## 🎛️ RAID Format State Matrix

The baseline release is deliberately narrow. RAID 0, RAID 1, and RAID 10 are the
production-supported rows; RAID 5/6 remain frozen for post-migration work.

| Format | State | Notes |
| --- | --- | --- |
| **RAID 0 (Stripe)** | ✅ **Baseline — stable** | Full horizontal data chunk striping. |
| **RAID 1 (Mirror)** | ✅ **Baseline — stable** | Duplicated writes across pairs; round-robin load-balanced reads; degraded failover + non-blocking resync. |
| **RAID 10 (Nested)** | ✅ **Baseline — stable** | Striped row segments over 2-way mirrored pairs (≥ 4 drives). Writes/discards mirror to both copies, reads round-robin, per-pair degraded failover. Pair-aware member rebuild/resync and boot-time degraded assembly landed; a lost pair refuses assembly. **Validated for production flash.** |
| RAID 5 (Parity) | 🧪 Experimental — frozen | Left-asymmetric rotating single-parity. |
| RAID 6 (Dual Parity) | 🧪 Experimental — frozen | P + Galois-field Q dual parity. |

---

## 🚀 Key Production Features

*   **Interactive Phase 1 Terminal GUI (TUI):** No pre-configured arrays are required on boot. On clean, blank testing drives, the live-installer automatically spins up a keyboard-driven blue menu canvas (`dialog`). Testers can check off raw NVMe paths using the Spacebar, choose a target level — **0, 1, or 10** for the production baseline (RAID 10 needs at least four disks; 5/6 remain frozen experimental) — and commit layouts on the fly.
*   **Defensive Hardware Security Guards:** The TUI contains an automated validation length check that enforces a hard **maximum 8-disk constraint** matching AMD firmware specifications, safely preventing memory overrun kernel oops before touching the PCIe bus registers.
*   **Advanced Rootfs Probing:** Phase 2 avoids fragile, hardcoded partition name assumptions. It uses a read-only sandboxed mount-and-probe matrix to peak inside array partitions, locate the genuine system `/etc/os-release`, and seamlessly execute chroot boot configurations on any distribution setup.
*   **Non-Blocking Asynchronous Completion Engine:** Parent-bio completion is parked on the `kamd_async_worker` kthread queue, so hardware completion contexts return immediately and never stall the block-layer queue; a separate `kamd_resync` thread owns long-running rebuilds. Over-reports from faulty member paths are absorbed by a bounded tombstone pool instead of being fatal.
*   **Persistent Boot Locks:** Automated **DKMS (Dynamic Kernel Module Support)** registration ensures the module automatically re-compiles against any incoming system kernel updates, paired with initramfs hooks and fallback `efibootmgr` NVRAM UEFI string injection.

---

## 🅐 Quick Start: Fresh OS Installation (From Live USB)

This is the recommended deployment path for setting up a fresh Linux environment (Ubuntu / Kubuntu / Mint / Fedora) directly onto an AMD hardware controller layout.

### Phase 1 — Provision Array and Expose Device
1. Boot into your live USB installer media environment, select **"Try"** (Live Session), and open a terminal window.
2. Clone this repository fork and execute the unified installation entry script:
   ```bash
   git clone https://github.com
   cd amd-raid-driver
   sudo ./install-livecd.sh
   ```
3. The script will automatically pull development dependencies in RAM, compile the driver, and launch the interactive **Terminal GUI**.
4. Use the **Arrow Keys** to navigate, **Spacebar** to check your member disks, and choose a baseline **RAID Level (0, 1, or 10)** — 5/6 remain frozen experimental.
5. Hit **OK**. The driver unbinds the raw paths, writes the layout data, and stands up **`/dev/rcraid0`** instantly.
6. Minimize the terminal and launch your desktop OS installer wizard. Select **Custom Partitioning**, map your layout over `/dev/rcraid0`, and let it copy files. **DO NOT REBOOT** when the installer finishes. Close the wizard panel and return to your open terminal window.

### Phase 2 — Target Chroot System Locking
1. Return to your active terminal where the installer script is waiting and press **Enter**.
2. The engine safely scans your partitions, mounts your target OS root filesystem, bind-mounts virtual filesystems, and chroots inside.
3. It installs DKMS, bakes the driver hooks straight into the new initramfs boot images, sets up structural udev definitions, and registers a fallback UEFI NVRAM string entry.
4. Once the message flashes `🎉 DONE!`, run **`sudo reboot`** to launch your fresh system straight from the array!

---

## ⚖️ License & Cleanroom Verification
This project is licensed under the **GPL-2.0-only** standard. It is a 100% clean-room, independently authored driver framework reverse-engineered from publicly distributed vendor binaries under DMCA §1201(f) interoperability protections—**no proprietary source code or confidential documentation was utilized.**
