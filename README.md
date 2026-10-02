# ⚡ rcraid (Multi-Format TUI-Enhanced Edition)

[![License: GPL-2.0-only](https://img.shields.io/badge/License-GPL--2.0--only-blue.svg)](https://opensource.org/license/gpl-2-0)
![Kernel: Linux 7.0+](https://img.shields.io/badge/Kernel-Linux%207.0%2B-orange.svg)
![DKMS: out-of-tree](https://img.shields.io/badge/DKMS-out--of--tree-informational.svg)

Your motherboard's AMD RAIDXpert2 NVMe storage arrays become a native, standard block device node right at **/dev/rcraid0**. Partition it, format it, configure it dynamically from a clean terminal wizard, and **boot Linux straight from it with zero graphical overhead.**

---

## 🚦 Deployment Status — Baseline Release

> **⚠️ DEPLOYMENT WARNING — READ BEFORE INSTALLING.**
> This fork is currently **locked to a functional, stable RAID 0 and RAID 1
> architecture pass**. The mirror completion and array rebuild paths run on a
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
*   **Frozen scope:** features outside the RAID 0 / RAID 1 baseline — the
    RAID 10 / RAID 5 / RAID 6 layouts and the remaining experimental engine
    code paths — are **intentionally frozen** for **post-migration execution
    phases**. They remain in-tree but are experimental; do not rely on them for
    production data.

---

## 🎛️ RAID Format State Matrix

The baseline release is deliberately narrow. Only the first two rows are
supported for production; the remainder are retained for post-migration work.

| Format | State | Notes |
| --- | --- | --- |
| **RAID 0 (Stripe)** | ✅ **Baseline — stable** | Full horizontal data chunk striping. |
| **RAID 1 (Mirror)** | ✅ **Baseline — stable** | Duplicated writes across pairs; round-robin load-balanced reads; degraded failover + non-blocking resync. |
| RAID 10 (Nested) | 🧪 Experimental — frozen | Striped row segments over mirrored pairs (≥ 4 drives). |
| RAID 5 (Parity) | 🧪 Experimental — frozen | Left-asymmetric rotating single-parity. |
| RAID 6 (Dual Parity) | 🧪 Experimental — frozen | P + Galois-field Q dual parity. |

---

## 🚀 Key Production Features

*   **Interactive Phase 1 Terminal GUI (TUI):** No pre-configured arrays are required on boot. On clean, blank testing drives, the live-installer automatically spins up a keyboard-driven blue menu canvas (`dialog`). Testers can check off raw NVMe paths using the Spacebar, choose a target level — **0 or 1** for the supported baseline (10/5/6 are experimental and frozen) — and commit layouts on the fly.
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
4. Use the **Arrow Keys** to navigate, **Spacebar** to check your member disks, and choose a baseline **RAID Level (0 or 1)** — 10/5/6 remain experimental and frozen.
5. Hit **OK**. The driver unbinds the raw paths, writes the layout data, and stands up **`/dev/rcraid0`** instantly.
6. Minimize the terminal and launch your desktop OS installer wizard. Select **Custom Partitioning**, map your layout over `/dev/rcraid0`, and let it copy files. **DO NOT REBOOT** when the installer finishes. Close the wizard panel and return to your open terminal window.

### Phase 2 — Target Chroot System Locking
1. Return to your active terminal where the installer script is waiting and press **Enter**.
2. The engine safely scans your partitions, mounts your target OS root filesystem, bind-mounts virtual filesystems, and chroots inside.
3. It installs DKMS, bakes the driver hooks straight into the new initramfs boot images, sets up structural udev definitions, and registers a fallback UEFI NVRAM string entry.
4. Once the message flashes `🎉 DONE!`, run **`sudo reboot`** to launch your fresh system straight from the array!

---

## 🧰 Developer Sandbox: Portable VirtualBox Emulation

The repository packages a portable development workspace companion inside the **`1022-b000/`** directory footprint. 

If you are modifying script logic or testing layout variations on your laptop before taking code down to bare-metal production rigs, you can use this extension pack module to mock an authentic AMD NVMe RAID Controller bus tree (`1022:B000`) natively inside VirtualBox.

### Deploying the Loose Development Pack
To bypass fragile zipped archive signature parsing locks (`VERR_PARSE_ERROR`), register the files straight as an unpacked loose development package:
```bash
# 1. Clean previous build paths and compile the C++ shared object library
make -C 1022-b000

# 2. Re-create VirtualBox's global system directory and link the workspace
sudo mkdir -p /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator
sudo cp -r 1022-b000/ExtPack.xml /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/
sudo mkdir -p /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/linux.amd64
sudo cp -r 1022-b000/linux.amd64/libmy_plugin.so /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/linux.amd64/
sudo chown -R root:root /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator

# 3. Enable hypervisor overrides and boot your headless test virtual instance
export VBOX_EXTPACK_ALLOW_UNSECURE=1
export VBOX_DEVELOPER_MODE=1
VBoxManage startvm "PluginTestVM" --type headless
```
The hypervisor Pluggable Device Manager (PDM) will natively recognize the `AmdRcraidEmulator` extension pack out of the box, allowing you to debug guest binding behaviors cleanly via your machine's `VBox.log` file.

---

## ⚖️ License & Cleanroom Verification
This project is licensed under the **GPL-2.0-only** standard. It is a 100% clean-room, independently authored driver framework reverse-engineered from publicly distributed vendor binaries under DMCA §1201(f) interoperability protections—**no proprietary source code or confidential documentation was utilized.**
