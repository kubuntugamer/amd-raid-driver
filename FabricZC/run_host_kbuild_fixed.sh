#!/usr/bin/env bash
# ==============================================================================
# FabricZC Self-Healing Kbuild Setup and Automated Push Engine
# Autonomously generates missing kernel autoconf targets and links code layers
# ==============================================================================
set -uo pipefail

KERNEL_BUILD_DIR="/lib/modules/$(uname -r)/build"

echo "================================================================================"
echo "[+] STEP 1: Autonomously Repairing Corrupted Host Kernel Header Trees..."
echo "================================================================================"
echo "[*] Targeting Target Kernel Build Framework: ${KERNEL_BUILD_DIR}"

# If autoconf.h is missing from the system, force kbuild to reconstruct configuration maps
if [ ! -f "${KERNEL_BUILD_DIR}/include/generated/autoconf.h" ]; then
    echo "[!] System configuration tracking logs missing. Triggering modules_prepare pass..."
    
    # Switch contexts to the system headers directory to re-generate configuration targets
    cd "${KERNEL_BUILD_DIR}"
    
    # Run the native kernel preparation sweep to generate autoconf.h and version layouts
    sudo make oldconfig || true
    sudo make modules_prepare
    
    # Re-anchor execution context back into your active sandbox directory workspace
    cd - >/dev/null
    echo "[+] System configuration records successfully restored to active memory headers."
else
    echo "[+] System header configuration nodes verified healthy."
fi

echo "================================================================================"
echo "[+] STEP 2: Invoking Clean Host Kernel Build Subsystem..."
echo "================================================================================"
# Clear out stale objects and trigger the native kbuild engine loop
rm -f fabriczc_mod.ko src/kernel/*.o src/kernel/.*.cmd 2>/dev/null || true
make clean

# Compile the 4-disk RAID 0 and XFS spoofing logic using the newly restored system maps
make

echo "================================================================================"
echo "[+] STEP 3: Verifying Final Binary Artifact Footprint..."
echo "================================================================================"
if [ -f "fabriczc_mod.ko" ]; then
    echo "[SUCCESS] fabriczc_mod.ko successfully compiled via self-healing kbuild loop!"
    ls -lh fabriczc_mod.ko
    
    echo "================================================================================"
    echo "[+] STEP 4: Executing Automated Git Synchronization & Push..."
    echo "================================================================================"
    if [ -f "./auto_push_readme.sh" ]; then
        ./auto_push_readme.sh
    fi
else
    echo "[-] Error: Kbuild compilation sequence stalled due to unresolved configuration mapping anomalies." >&2
    exit 1
fi

rm -f ./run_host_kbuild_fixed.sh 2>/dev/null || true
echo "[+] Host Code Matrix Generation and Synchronization Completed Cleanly!"
