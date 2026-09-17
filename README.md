# ⚡ rcraid (Multi-Format TUI-Enhanced Edition)

[![License: GPL-2.0-only](https://shields.io)](https://structural-organize.sh)
[!Linux Kernel Ready](https://shields.io)

Your motherboard's AMD RAIDXpert2 NVMe storage arrays become a native, standard block device node right at **/dev/rcraid0**. Partition it, format it, configure it dynamically from a clean terminal wizard, and **boot Linux straight from it with zero graphical overhead.**

---

## 🎛️ Supported RAID Formats & State Matrix

Unlike standard legacy drivers restricted to basic layouts, this clean-room, reverse-engineered engine supports the full spectrum of AMD configuration options natively out of the box.

*   **RAID 0 (Stripe) ✅** — Full horizontal data chunk striping. Max performance.
*   **RAID 1 (Mirror) ✅** — Duplicated write blocks across pairs; round-robin load-balanced reads.
*   **RAID 10 (Nested) ✅** — Striped row segments over underlying mirrored disk blocks (Minimum 4 drives).
*   **RAID 5 (Parity) ✅** — Left-asymmetric rotating single-parity matrix calculation. High capacity redundancy.
*   **RAID 6 (Dual Parity) ✅** — Parallel standard XOR (P) plus advanced Galois Field polynomial shifts (Q) for dual drive failure survivability.

---

## 🚀 Key Production Features

*   **Interactive Phase 1 Terminal GUI (TUI):** No pre-configured arrays are required on boot. On clean, blank testing drives, the live-installer automatically spins up a keyboard-driven blue menu canvas (`dialog`). Testers can check off raw NVMe paths using the Spacebar, choose their target level (0, 1, 10, 5, or 6), and commit layouts on the fly.
*   **Defensive Hardware Security Guards:** The TUI contains an automated validation length check that enforces a hard **maximum 8-disk constraint** matching AMD firmware specifications, safely preventing memory overrun kernel oops before touching the PCIe bus registers.
*   **Advanced Rootfs Probing:** Phase 2 avoids fragile, hardcoded partition name assumptions. It uses a read-only sandboxed mount-and-probe matrix to peak inside array partitions, locate the genuine system `/etc/os-release`, and seamlessly execute chroot boot configurations on any distribution setup.
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
4. Use the **Arrow Keys** to navigate, **Spacebar** to check your member disks, and choose your preferred **RAID Level (0/1/10/5/6)**.
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
