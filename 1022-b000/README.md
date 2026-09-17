# AMD RCRAID VirtualBox Emulator (`1022-b000`)

This subdirectory contains a self-contained, auditable development workspace to emulate an **AMD NVMe RAID Bottom Controller** inside Oracle VirtualBox.

## Purpose
When testing this out-of-tree `rcraid` driver module inside a VirtualBox guest virtual machine, the driver will normally fail to bind because standard virtual NVMe storage adapters present an Intel hardware signature (`8086:5845`). 

This extension module hooks into VirtualBox's hardware virtualization layer. The moment a virtual machine containing an attached NVMe drive boots, this plugin automatically overrides the configuration registers to present authentic AMD signatures:
* **Vendor ID:** `0x1022` (Advanced Micro Devices, Inc.)
* **Device ID:** `0xB000` (AMD NVMe RAID Bottom Controller)

This allows testing of the driver's block I/O request paths, stripe boundaries, and RAIDXpert metadata configurations on a completely stock guest operating system environment without modifying production device tables in the kernel source code.

---

## Technical Caveats & Quirks
* **Loose Directory Layout:** VirtualBox enforces a highly rigid cryptographic checksum format on zipped `.vbox-extpack` archives that makes local compilation testing fragile. To bypass these parsing blocks entirely, this plugin is deployed as an **unpacked development extension folder**. 
* **Global Interception:** The module registers itself over VirtualBox's internal `"NVMe"` device class. As a result, **any standard NVMe storage controller attached to a guest VM on this host will automatically be spoofed as an AMD RCRAID controller.**

---

## Why Administrator Escalation (`sudo`) is Required
VirtualBox implements a strict privilege-separation model on Linux hosts to prevent standard user spaces from injecting unauthorized binaries into the hypervisor execution path:
1. **Protected Path Access:** VirtualBox only evaluates extensions located inside the global system folder `/usr/lib/virtualbox/ExtensionPacks/`. Standard user accounts do not have write permissions to this directory tree without `sudo`.
2. **Ownership Validation:** On machine initialization, VirtualBox scans all global extension binaries. If any compiled shared object (`.so`) file is owned or modifiable by a standard user instead of `root`, the hypervisor flags it as a security hazard and refuses to load it. The installation process uses `sudo chown` to satisfy this internal restriction.

---

## Installation & Test Instructions

Execute these commands from the root of your cloned repository to build and register the emulator plugin locally:

### 1. Configure the Host Environment
Instruct the local hypervisor to authorize unsigned developer extension modules globally:
```bash
echo "VBOX_EXTPACK_ALLOW_UNSECURE=1" | sudo tee -a /etc/default/virtualbox > /dev/null
echo "VBOX_DEVELOPER_MODE=1" | sudo tee -a /etc/default/virtualbox > /dev/null
```

### 2. Compile the Shared Object Binary
Navigate into this subdirectory and compile the clean, auditable source code:
```bash
cd 1022-b000
make clean && make
```

### 3. Deploy the Unpacked Extension Pack
Create the system-wide extension path directory, copy the operational layouts into place, and lock down secure administrative permissions:
```bash
# Establish the target system directory structure
sudo mkdir -p /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/linux.amd64

# Copy configuration manifests and compiled binaries
sudo cp ExtPack.xml /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/
sudo cp linux.amd64/libmy_plugin.so /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator/linux.amd64/

# Align file ownership with hypervisor security requirements
sudo chown -R root:root /usr/lib/virtualbox/ExtensionPacks/AmdRcraidEmulator
```

### 4. Verify and Boot
Initialize your local environment overrides and launch your virtual machine. Replace `"Your_VM_Name"` with your active test instance identifier:
```bash
export VBOX_EXTPACK_ALLOW_UNSECURE=1
export VBOX_DEVELOPER_MODE=1
VBoxManage startvm "Your_VM_Name" --type headless
```
