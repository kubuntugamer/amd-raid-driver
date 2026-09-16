# AMD RAIDXpert2 User Space Management Console (rcraidXpert)

This directory contains the un-mocked, pure hardware graphical configuration utility (`rcraidXpert`) designed to interact with the out-of-tree AMD NVMe RAID storage kernel driver via the `/dev/amd_ctl0` interface plane.

## Platform Requirements & Driver State

This utility communicates strictly with bare-metal hardware. For the user interface to populate categories, discover hard drives, or manage arrays, the system must satisfy the following state conditions:
1. **Driver Loaded:** The underlying storage driver (`rcraid.ko`) must be actively loaded into kernel memory.
2. **Device Present:** The character control bridge multiplexer file path `/dev/amd_ctl0` must exist with valid read/write block permissions.

---

## How to Launch the GUI (Automated Method)

To accommodate testing inside minimal **Live USB environments** (where core graphical libraries are missing out-of-the-box), a self-contained environment configuration script is provided. This script automatically detects the host OS, silently pulls down required dependencies, verifies the driver state, and kicks off the binary interface.

Open a terminal inside this directory and execute the following single command string:

```bash
./launch-liveusb.sh
```

---

## Manual Execution Track

If you prefer to configure your environment paths manually inside your live session workspace, execute the specific package installation string for your active operating system, ensure the driver is active, and invoke the binary directly:

### 1. Install Runtime Dependencies

* **Ubuntu / Kubuntu / Linux Mint:**
  ```bash
  sudo apt update && sudo apt install -y libqt6widgets6 qt6-base-plugins
  ```
* **Fedora Linux:**
  ```bash
  sudo dnf install -y qt6-qtbase-gui
  ```
* **Arch Linux / CachyOS:**
  ```bash
  sudo pacman -Sy --noconfirm qt6-base
  ```

### 2. Verify Driver Infrastructure Bounds
Ensure your out-of-tree testing driver module is loaded to expose the inter-process communication bridge before launching the interface canvas:
```bash
sudo modprobe rcraid
```

### 3. Invoke the Production Executable
```bash
chmod +x ./rcraidXpert
./rcraidXpert
```

---

## Interface Topology Guide

* **Left Navigation Tree:** Manages real-time hardware discovery sweeps. If the driver is initialized successfully, it maps `Physical Drives`, `Logical Drives`, and active `Arrays` dynamically.
* **Array Management Properties:** Provides a 3-column physical layout matrix (`Status`, `ID`, `Capacity`) mapping out-of-tree storage structures cleanly. Assigned disk members render a **Solid Blue Block**, whereas unassigned drives render a **Solid Green Block** to match authentic hardware profiles.
* **System Events Monitor Console:** Tracks block device events, configuration commits, and hardware exception handling metrics.
