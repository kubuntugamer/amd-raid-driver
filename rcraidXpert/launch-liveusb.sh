#!/bin/bash
set -euo pipefail

echo "=== AMD RAIDXpert2 LIVE USB ENVIRONMENT INITIALIZATION ==="

# 1. Detect the running Linux distribution platform cleanly
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_ID=${ID:-}
    OS_LIKE=${ID_LIKE:-}
else
    echo "[ERROR] Cannot detect operating system profile. Aborting."
    exit 1
fi

echo "[INFO] Active Live Session Platform Detected: ${NAME:-unknown}"

# 2. Qt6 development packages, needed because the GUI is built from source
echo "[INFO] Checking Qt6 development toolchain..."
if [[ "$OS_ID" == "ubuntu" || "$OS_ID" == "linuxmint" || "$OS_LIKE" == *"ubuntu"* ]]; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq cmake build-essential qt6-base-dev
elif [[ "$OS_ID" == "fedora" || "$OS_LIKE" == *"fedora"* ]]; then
    sudo dnf install -y -q cmake gcc-c++ qt6-qtbase-devel
elif [[ "$OS_ID" == "arch" || "$OS_ID" == "cachyos" || "$OS_LIKE" == *"arch"* ]]; then
    sudo pacman -Sy --noconfirm -q cmake qt6-base
else
    echo "[WARN] Unknown package management track. Assuming cmake and Qt6 are installed."
fi

# 3. Non-negotiable hardware interface validation sweeps
echo "[INFO] Verifying inter-process bridge control paths..."
if [ ! -c "/dev/amd_ctl0" ]; then
    echo "[WARN] Device /dev/amd_ctl0 sits unprovisioned. Checking driver module state..."

    # Attempt to locate and insert your natively compiled out-of-tree driver module locally first
    if [ -f "../rcraid.ko" ]; then
        echo "[INFO] Local driver binary target detected. Loading rcraid.ko module..."
        sudo insmod ../rcraid.ko
    elif [ -f "./rcraid.ko" ]; then
        sudo insmod ./rcraid.ko
    else
        echo "[INFO] Falling back to system modprobe lookup track..."
        sudo modprobe rcraid 2>/dev/null || true
    fi

    # Final validation sweep block to prevent interface configuration failures
    if [ ! -c "/dev/amd_ctl0" ]; then
        echo "[CRITICAL] Hardware interface layer unavailable. The AMD driver failed to link."
        echo "           Ensure Secure Boot is not actively blocking unsigned module injection."
        exit 1
    fi
fi

# 4. Build the GUI from this tree (there is no prebuilt binary in the repo)
BUILD_DIR=${BUILD_DIR:-build}
if [ ! -x "$BUILD_DIR/rcraidXpert" ]; then
    echo "[INFO] Building rcraidXpert from source into $BUILD_DIR..."
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "[WARN] Running as uid $(id -u): inventory and status are readable, but"
    echo "       creating or deleting an array needs root (CAP_SYS_ADMIN)."
    echo "       Re-run with sudo to enable those actions."
fi

echo "[SUCCESS] Control bridge verified. Launching rcraidXpert..."
exec "./$BUILD_DIR/rcraidXpert" "$@"
