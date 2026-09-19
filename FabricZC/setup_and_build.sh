#!/usr/bin/env bash
# ==============================================================================
# FabricZC Automated Live USB Dependency Installer & Driver Compiler
# Target Execution Environment: Guest VM Live USB Console Session
# ==============================================================================
set -euo pipefail

echo "================================================================================"
echo "[🎉] INITIALIZING FABRICZC AUTOMATED LAB RUNNER SECTION..."
echo "================================================================================"

echo "[+] STEP 1: Updating System Repositories & Injecting Development Packages..."
echo "================================================================================"
# Elevate privilege boundaries to pull baseline build tool footprints into volatile memory
sudo apt-get update
sudo apt-get install -y build-essential gcc make git linux-headers-$(uname -r)

echo "================================================================================"
echo "[+] STEP 2: Cleaning Workspace and Aligning Directory Nodes..."
echo "================================================================================"
# Scrub away any stale host tracking objects from previous execution paths
rm -f fabriczc_mod.ko src/kernel/*.o src/kernel/.*.cmd 2>/dev/null || true

echo "================================================================================"
echo "[+] STEP 3: Invoking Native Kbuild Compilation Loop..."
echo "================================================================================"
# Trigger the local Makefile targets using the active guest environment headers
make clean
make

echo "================================================================================"
echo "[+] STEP 4: Verifying Final Driver Binary Artifact Generation..."
echo "================================================================================"
if [ -f "fabriczc_mod.ko" ]; then
    echo "================================================================================"
    echo "[🎉 SUCCESS] FABRICZC DRIVER COMPILED CLEANLY INSIDE THE LIVE USB!"
    echo "================================================================================"
    ls -lh fabriczc_mod.ko
    echo ""
    echo "[*] Ready for hardware takeover. To insert the module right now, type:"
    echo "    sudo insmod fabriczc_mod.ko"
    echo "================================================================================"
else
    echo "[-] Error: Linkage verification phase failed due to underlying path restrictions." >&2
    exit 1
fi
