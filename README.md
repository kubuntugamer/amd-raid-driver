# ⚡ rcraid (Multi-Format TUI-Enhanced Edition)

[![License: GPL-2.0-only](https://img.shields.io/badge/License-GPL--2.0--only-blue.svg)](https://opensource.org/license/gpl-2-0)
![Kernel: Linux 7.0+](https://img.shields.io/badge/Kernel-Linux%207.0%2B-orange.svg)
![DKMS: out-of-tree](https://img.shields.io/badge/DKMS-out--of--tree-informational.svg)

Your motherboard's AMD RAIDXpert2 NVMe storage arrays become a native, standard block device node right at **/dev/rcraid0**. Partition it, format it, configure it dynamically from a clean terminal wizard, and **boot Linux straight from it with zero graphical overhead.**

---

## 🚦 Deployment Status — Baseline Release (2026-10-02)

> **⚠️ DEPLOYMENT WARNING — READ BEFORE INSTALLING.**
> The hardware-validated baseline is **RAID 0 and RAID 1**. **RAID 10 is
> feature-complete** — reads/writes/discards, per-pair degraded failover, member
> rebuild, and boot-time degraded assembly are all implemented — **but it has not had
> its own array-box run yet**, so treat it as unproven for production data until
> [`docs/VALIDATION_RAID10.md`](docs/VALIDATION_RAID10.md) is signed off.
> **RAID 5/6** assemble **read-only**: the driver identifies them and reads
> them, but the parity write path is not implemented and the parity map, while
> now matching the vendor (dedicated parity on the last member, found by
> reverse engineering), has not been checked against an array the vendor wrote.
> Not deployable yet.
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
*   **Real-firmware corroboration (2026-10-07):** the metadata format and the RAID 10 geometry model were validated against four raw member dumps from a firmware-created X399 array (George "PlaidPiper" Crossley, [`PlaidPiper/x399-raidcore-fixtures`](https://github.com/PlaidPiper/x399-raidcore-fixtures), reported in [joeytroy#57](https://github.com/joeytroy/amd-raid-driver/issues/57) and cross-linked from [issue #1](https://github.com/kubuntugamer/amd-raid-driver/issues/1)). All four dumps parse clean through this driver's validation chain, and a byte-level cross-dump of the members confirms the mirror-pair convention is **adjacent** (pairs `{0,1}` / `{2,3}`) — the last unverified RAID 10 assumption. Details in [`docs/REVERSE_ENGINEERING.md`](docs/REVERSE_ENGINEERING.md).
*   **Fault isolation hardened:** all transient device fault-isolation loops now cleanly catch and handle `BLK_STS_RESOURCE` boundaries, guaranteeing total array resilience against unexpected hardware dropouts. The async completion engine returns `BLK_STS_RESOURCE` on allocation failure or unload race, never `BLK_STS_IOERR`, so the block layer retries instead of reporting false I/O errors to the filesystem.
*   **RAID 5/6: recognised and READABLE, not writable (2026-11-08).** The level is taken from the LD `DeviceType` alone (`0x1BF5` RAID5, `0x1BFA` RAID6) — `FirstCount × SecondCount` cannot express a parity layout, so a count-encoded `0x1BF6` record is never mistaken for one, and a RAID 6 record can no longer be assembled as RAID 10. The parity encoding is now **confirmed against a BIOS-created 3-disk RAID 5** (capacity `2 ×` per-member user size, `DeviceType 0x1BF5`); an earlier revision had the two values swapped from the dispatch-table inference. A parity volume assembles, reports the right capacity (`(N-1)` / `(N-2)` × per-member user size) and serves reads through the shared map in `src/patch_parity_math.c`; writes and discards are refused and the volume is forced read-only. The parity **placement** is settled from the vendor core — **dedicated parity on member `N-1`, no rotation** (issue #6) — and the RAID 6 **Q encoding** is settled too: **EVENODD/RDP diagonal parity with prime 17 and no GF arithmetic at all** (issue #3) — and the generator now implements exactly that (`src/raid6_evenodd.h`, issue #10), with the Galois code gone; it still has no caller while the volume stays read-only. The layout itself rests on AMD's code rather than on inference from one array, so what remains is *corroboration* from a vendor-written array — not an open question about the map — plus the parity write path, rebuild, and metadata writer.
*   **Enterprise roadmap (this driver, not experimental):**
    *   **OCE (Online Capacity Expansion)** — add drives to RAID 5/6 arrays online, restripe parity, grow filesystems without downtime.
    *   **ORLM (Online RAID Level Migration)** — migrate RAID 0↔5↔6↔10 online, zero host CPU, instant metadata commit.
    *   These are **first-class roadmap items for this driver** — not experimental, not imported. Enterprise management ops are first-class citizens.
*   **Frozen scope:** remaining experimental engine code paths (FabricZC, HighPoint metadata import) are **intentionally frozen** for **post-migration execution phases**. They remain in-tree but are experimental; do not rely on them for production data. **Prime directive: nothing from experimental migrates to main.**

---

## 🗺️ Enterprise Roadmap (This Driver — Not Experimental)

Phases 0–8 — including **OCE**, **ORLM**, COW snapshots, P2PDMA, io_uring and
ZNS/FDP offload — are tracked in **[`docs/IMPLEMENTATION.MD`](docs/IMPLEMENTATION.MD)**
under "Long-horizon phases", with one status per phase. That file is the
single source of truth for both the long view and the near-term order; the
table is deliberately not duplicated here, so the two can't drift apart.

Where the phases actually stand: **Phase 0** (RAID 0/1/10 + rebuild) is done
for RAID 0/1 and awaiting the RAID 10 array-box run; **Phase 1** (parity)
assembles and reads but refuses writes; **Phase 2** (metadata writer + TUI
create) is the next PR — the TUI can select 10/5/6 today, but the metadata
writer only produces 0/1, so those choices stop at an error instead of
stamping a layout the driver can't validate; phases 3–8 are long-horizon.

**Prime directive: nothing from experimental branch migrates to main. OCE/ORLM/P2PDMA/io_uring are first-class features of *this* driver.**

---

## 🎛️ RAID Format State Matrix

The baseline release is deliberately narrow. RAID 0 and RAID 1 are the
production-supported, hardware-validated rows; RAID 10 is feature-complete and
next in line for its array-box run; RAID 5/6 assemble and read, but refuse every
write until the parity write path and the layout itself are confirmed.

| Format | State | Notes |
| --- | --- | --- |
| **RAID 0 (Stripe)** | ✅ **Baseline — stable** | Full horizontal data chunk striping. |
| **RAID 1 (Mirror)** | ✅ **Baseline — stable** | Duplicated writes across pairs; round-robin load-balanced reads; degraded failover + non-blocking resync. |
| **RAID 10 (Nested)** | 🟡 **Feature-complete — validation pending** | Striped row segments over 2-way mirrored pairs (≥ 4 drives). Writes/discards mirror to both copies, reads round-robin, per-pair degraded failover. Pair-aware member rebuild/resync and boot-time degraded assembly landed; a lost pair refuses assembly. Mirror-pair convention (adjacent `{2c, 2c+1}`) confirmed against real X399 firmware dumps (2026-10-07), but the NVMe `1022:B000` data path has **not** had its own array-box run — the [runbook's](docs/VALIDATION_RAID10.md) results table is still empty. |
| RAID 5 (Parity) | 🟡 **Read-only — assembly + reads wired** | Single-parity, dense per-row packing. Level comes from the LD `DeviceType` only (`0x1BF5`, confirmed on a BIOS-created 3-disk array 2026-11-08). Volume assembles at `(N-1)` × per-member capacity and serves reads; writes/discards are refused and the queue is capped at one stripe per request, because the parity-chunk address is the *same* row slot as the data chunk it protects. **Level/geometry hardware-confirmed.** Parity placement is settled from the vendor blob — **dedicated parity on member `N-1`, no rotation** — and the map (`rc_amd_map_distributed_raid5`) now implements it; the earlier row-rotating map disagreed on two rows in three and was replaced 2026-11-08 ([VALIDATION_RAID5.md](docs/VALIDATION_RAID5.md), issue #6). **Layout settled against the vendor implementation**; not yet corroborated by reading a vendor-written array, and member order comes from the LD element array. The volume is read-only because there is no parity write path, not because the map is in doubt. No rebuild or metadata writer yet. |
| RAID 6 (Dual Parity) | 🟡 **Read-only — assembly + reads wired** | P + Q dual parity (`0x1BFA`); data on `0..N-3` with P/Q dedicated to `N-2`/`N-1`, matching the vendor blob (the P/Q order within those two is assumed). Same read-only contract as RAID 5, capacity `(N-2)` × per-member. The vendor's Q is **EVENODD/RDP diagonal parity (prime 17, XOR plus an integer modulus, no Galois field)**, settled from `rc_raid6_evenodd.cpp` (issue #3) — and the generator in this tree implements that construction (`src/raid6_evenodd.h`, issue #10), with no Galois code at all. It still has no callers. No reconstruct/verify path exists, so nothing reads parity yet; a healthy array is read from its data members alone. |

---

## 🚀 Key Production Features

*   **Interactive Phase 1 Terminal GUI (TUI):** No pre-configured arrays are required on boot. On clean, blank testing drives, the live-installer automatically spins up a keyboard-driven blue menu canvas (`dialog`). Testers can check off raw NVMe paths using the Spacebar, choose a target level — **0, 1, 10, 5, or 6** (**RAID 5/6 come up read-only — their parity write path isn't wired**, so those levels can only be read; RAID 10 needs at least four disks and is awaiting hardware validation) — and commit layouts on the fly. Layout metadata is written by `scripts/qemu-test/mkmeta.py`, which covers **RAID 0 and 1** only: selecting 10/5/6 stops with an explicit message instead of stamping a layout the driver cannot validate.
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
   git clone https://github.com/kubuntugamer/amd-raid-driver.git
   cd amd-raid-driver
   sudo ./install-livecd.sh
   ```
3. The script pulls development dependencies in RAM and compiles the driver, then looks for an array **RAIDXpert2 already created**: it reads the RAIDCore metadata at LBA `0x5000` and, when it is there, takes exactly those disks as members and writes nothing to them. Level and geometry come from the firmware's records — RAID 10/5/6 included.
4. Only when no disk carries that metadata does it open the **Terminal GUI**: **Spacebar** to check member disks and RAID **0 or 1** (the only levels `mkmeta.py` can write — **5/6 assemble read-only, their parity write path isn't wired**; RAID 10 needs four disks and is awaiting hardware validation). That path *stamps fresh metadata* over the selected disks; with a firmware array the menu never appears.
5. Either way the driver unbinds the members from `nvme`, binds them to `rcbottom`, and stands up **`/dev/rcraid0`**.
6. Minimize the terminal and launch your desktop OS installer wizard. Select **Custom Partitioning**, map your layout over `/dev/rcraid0`, and let it copy files. **DO NOT REBOOT** when the installer finishes. Close the wizard panel and return to your open terminal window.

### Phase 2 — Target Chroot System Locking
1. Return to your active terminal where the installer script is waiting and press **Enter**.
2. The engine safely scans your partitions, mounts your target OS root filesystem, bind-mounts virtual filesystems, and chroots inside.
3. It installs DKMS, bakes the driver hooks straight into the new initramfs boot images, sets up structural udev definitions, and registers a fallback UEFI NVRAM string entry. If you accept its offer to enrol a module-signing key, the driver is signed too, so Secure Boot can be switched back on later.
4. Once the message flashes `🎉 DONE!`, run **`sudo reboot`** to launch your fresh system straight from the array!
5. With a signing key enrolled, that first boot stops at shim's **MOK management** screen: **Enroll MOK → Continue → Yes**, type the one-time password the installer printed, and let it reboot. (The live session itself has to run with Secure Boot **off** — the installer refuses to start otherwise, because the live kernel rejects the unsigned module and the key it needs cannot be enrolled without rebooting the session. See `docs/INSTALL.md`, "Secure Boot".)

---

## ⚖️ License & Cleanroom Verification
This project is licensed under the **GPL-2.0-only** standard. It is a 100% clean-room, independently authored driver framework reverse-engineered from publicly distributed vendor binaries under DMCA §1201(f) interoperability protections—**no proprietary source code or confidential documentation was utilized.**
