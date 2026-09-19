#!/usr/bin/env bash
# ==============================================================================
# FabricZC Live USB Kernel Synchronization & Rebuild Agent
# Pulls missing environment headers and re-syncs vermagic to match live memory
# ==============================================================================
set -uo pipefail

echo "================================================================================"
echo "[+] STEP 1: Installing Missing Live USB Compilation Header Tools..."
echo "================================================================================"
# Pull down the exact matching kernel compilation headers for this active live session
sudo apt update
sudo apt install -y linux-headers-$(uname -r) build-essential gcc make

echo "================================================================================"
echo "[+] STEP 2: Dynamically Extracting Active Live Kernel Vermagic..."
echo "================================================================================"
LIVE_KERNEL_VERSION="$(uname -r)"
echo "[*] Active Live Kernel Version Detected: ${LIVE_KERNEL_VERSION}"

# Re-forge the standalone module metadata block to match the running VM kernel signature
cat << INNER_EOF > src/kernel/fabriczc_mod.mod.c
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, "${LIVE_KERNEL_VERSION} SMP preempt mod_unload");
MODULE_INFO(name, "fabriczc_mod");
INNER_EOF

echo "================================================================================"
echo "[+] STEP 3: Executing Clean Out-of-Tree Sandbox Build Pass..."
echo "================================================================================"
rm -f fabriczc_mod.ko src/kernel/*.o src/kernel/.*.cmd 2>/dev/null || true

SRC_HEADER_DIR="/usr/src/linux-headers-${LIVE_KERNEL_VERSION}"

# Compile the core logic block using the freshly installed environment header paths
gcc -nostdinc -std=gnu11 \
    -I"$(pwd)/staging_includes" \
    -I"${SRC_HEADER_DIR}/include" \
    -I"${SRC_HEADER_DIR}/arch/x86/include" \
    -D__KERNEL__ -DMODULE \
    -O2 -Wall -m64 -mcmodel=kernel -mno-red-zone -fno-pie \
    -c src/kernel/main.c -o src/kernel/main.o

# Compile the newly synced vermagic metadata node
gcc -nostdinc -std=gnu11 \
    -I"$(pwd)/staging_includes" \
    -I"${SRC_HEADER_DIR}/include" \
    -D__KERNEL__ -DMODULE \
    -O2 -fno-pie \
    -c src/kernel/fabriczc_mod.mod.c -o src/kernel/fabriczc_mod.mod.o

# Link the final corrected binary object payload
ld -r -m elf_x86_64 -z max-page-size=0x1000 -o fabriczc_mod.ko src/kernel/main.o src/kernel/fabriczc_mod.mod.o

echo "================================================================================"
echo "[+] STEP 4: Triggering Final Insertion Test Pass..."
echo "================================================================================"
if [ -f "fabriczc_mod.ko" ]; then
    echo "[+] SUCCESS: fabriczc_mod.ko rebuilt cleanly for active live environment!"
    ls -lh fabriczc_mod.ko
    
    echo "[*] Attempting live insertion takeover loop..."
    sudo insmod fabriczc_mod.ko
    
    if lsmod | grep -q fabriczc_mod; then
        echo "[🎉 SUCCESS] FabricZC module successfully inserted into the live kernel!"
        sudo dmesg | tail -n 10
    fi
else
    echo "[-] Error: Rebuild linkage phase failed." >&2
    exit 1
fi

rm -f ./fix_live_usb_compile.sh 2>/dev/null || true
