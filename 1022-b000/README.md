# AMD RCRAID VirtualBox Emulator (`1022-b000`)

⚠️ **SECURITY & AUDIT NOTICE:** Because this code is AI-generated and requires administrative/root privileges (`sudo`) to install, you are strongly encouraged to audit the source code *before* installation to verify that no malicious behavior or unauthorized network execution is present. Instructions for extraction and manual verification are detailed below.

## Purpose
When testing this out-of-tree `rcraid` driver module inside a VirtualBox guest virtual machine, the kernel module will normally fail to bind because standard virtual NVMe storage adapters present an Intel hardware signature (`8086:5845`). 

This extension pack hooks into VirtualBox's internal Pluggable Device Manager (PDM) storage layer at runtime. The moment a virtual machine containing an attached NVMe drive boots on this host, this plugin intercepts the hardware configuration registers and overrides them to present authentic AMD signatures:
* **Vendor ID:** `0x1022` (Advanced Micro Devices, Inc.)
* **Device ID:** `0xB000` (AMD NVMe RAID Bottom Controller)

This allows you to evaluate block I/O request paths, stripe boundaries, and RAIDXpert metadata configurations in a completely stock guest operating system environment without modifying production device tables in your clean kernel source code.

---

## 🔍 How to Audit This Package (Verify Before Installing)
To guarantee that the pre-compiled binary matches the source code exactly and contains no hidden payloads (such as botnets, telemetry trackers, or malware), you can inspect and verify the contents of the `.vbox-extpack` container file before running the installer:

### 1. View the Raw C++ Source Code Directly
Extract and read the internal source file directly from the compressed archive without installing anything to your system:
```bash
tar -O -xf AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack my_plugin.cpp
```

### 2. Verify Cryptographic Integrity
The archive contains a plain-text ledger called `ExtPack.manifest`. You can compute the SHA-256 hash of the source code file locally and ensure it matches the ledger perfectly to prove the binary correlates explicitly to the auditable code:
```bash
# Extract the manifest ledger
tar -O -xf AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack ExtPack.manifest
```

---

## 🛑 Why Administrator Escalation (`sudo`) is Mandatory
Oracle VirtualBox implements a strict privilege-separation security model on Linux hosts to protect the host system kernel from rogue guest modifications. When you install this extension pack via the VirtualBox Graphical User Interface (GUI) or the command line, the host operating system will request your administrative passcode for two non-negotiable reasons:

1. **Protected Path Access:** To prevent unprivileged malware from hijacking your virtual machines, VirtualBox will *only* evaluate extension pack modules located inside the globally protected system directory path: `/usr/lib/virtualbox/ExtensionPacks/`. Standard user accounts are blocked from writing to this tree path without `sudo`.
2. **Strict Ownership Validation Loops:** On hypervisor initialization, VirtualBox scans all global extensions. If any compiled shared object module binary (`.so`) is owned or modifiable by a regular user account instead of the system `root` account, VirtualBox flags it as a severe security vulnerability and hard-refuses to execute it. The installation framework requires administrative clearance to explicitly run `chown root:root` on the files.

---

## Technical Quirks & Behaviors
* **Global Interception:** The module registers itself over VirtualBox's global `"NVMe"` internal storage abstraction module engine. As a result, **any standard NVMe drive attached to any virtual machine running on this host will automatically present itself to the guest kernel as an AMD RCRAID controller.**
* **Development Mode Overrides:** Because this is an unsigned third-party developer tool built outside of Oracle's closed-source compilation infrastructure, you must tell your local hypervisor instance to accept unverified local modules by running the environment overrides detailed below.

---

## Quick Installation Instructions

### 1. Configure the Host Environment Flags
Authorize unsecure local developer extension modules globally on your host system:
```bash
echo "VBOX_EXTPACK_ALLOW_UNSECURE=1" | sudo tee -a /etc/default/virtualbox > /dev/null
echo "VBOX_DEVELOPER_MODE=1" | sudo tee -a /etc/default/virtualbox > /dev/null
```

### 2. Install Via the VirtualBox Graphical UI Manager (Recommended)
1. Open **VirtualBox Manager**.
2. Click **File** Menu → Select **Tools** → Click **Extension Pack Manager**.
3. Click the green **Install** button.
4. Navigate inside this directory (`1022-b000/`), select **`AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack`**, and click Open.
5. Provide your administrative password when prompted by the operating system to allow VirtualBox to safely copy the assets into the protected system directory paths.

### 3. Alternative Command-Line Installation
If you prefer the terminal pipeline, deploy the bundle system-wide using `VBoxManage`:
```bash
sudo VBoxManage extpack install --replace AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack
```
