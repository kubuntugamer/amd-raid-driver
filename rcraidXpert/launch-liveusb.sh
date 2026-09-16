#!/bin/bash

echo "=== AMD RAIDXpert2 LIVE USB ENVIRONMENT INITIALIZATION ==="

# 1. Detect the running Linux distribution platform cleanly
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_ID=$ID
    OS_LIKE=$ID_LIKE
else
    echo "[ERROR] Cannot detect operating system profile. Aborting."
    exit 1
fi

echo "[INFO] Active Live Session Platform Detected: ${NAME}"

# 2. Automated silent package installation pipeline based on OS profiles
echo "[INFO] Running dynamic package check for required Qt6 runtime libraries..."
if [[ "$OS_ID" == "ubuntu" || "$OS_ID" == "linuxmint" || "$OS_LIKE" == *"ubuntu"* ]]; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq libqt6widgets6 qt6-base-plugins
elif [[ "$OS_ID" == "fedora" || "$OS_LIKE" == *"fedora"* ]]; then
    sudo dnf install -y -q qt6-qtbase-gui
elif [[ "$OS_ID" == "arch" || "$OS_ID" == "cachyos" || "$OS_LIKE" == *"arch"* ]]; then
    sudo pacman -Sy --noconfirm -q qt6-base
else
    echo "[WARN] Unknown package management track. Attempting to run with fallback options..."
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
        sudo modprobe rcraid 2>/dev/null
    fi

    # Final validation sweep block to prevent interface configuration failures
    if [ ! -c "/dev/amd_ctl0" ]; then
        echo "[CRITICAL] Hardware interface layer unavailable. The AMD driver failed to link."
        echo "           Ensure Secure Boot is not actively blocking unsigned module injection."
        exit 1
    fi
fi

echo "[SUCCESS] Control bridge verified. Launching AMD RAIDXpert2 Management Suite Console..."
chmod +x ./rcraidXpert
./rcraidXpert
