#!/usr/bin/env bash
# ==============================================================================
# FabricZC Native Host Kbuild Wrapper & Push Engine
# Utilizes the native Linux kbuild subsystem to ensure flawless header mappings
# ==============================================================================
set -uo pipefail

echo "================================================================================"
echo "[+] STEP 1: Deploying Isolated Linux Kbuild Makefile..."
echo "================================================================================"

# Write a clean, standard kernel Makefile to let the host subsystem resolve the headers
cat << 'INNER_EOF' > Makefile
obj-m += fabriczc_mod.o
fabriczc_mod-y := src/kernel/main.o

# Force include our custom staging headers path during the kbuild execution pass
ccflags-y := -I$(src)/staging_includes -std=gnu11 -Wno-unused-variable

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
INNER_EOF

echo "[+] Native Makefile established successfully."

echo "================================================================================"
echo "[+] STEP 2: Invoking Host Kernel Build Subsystem..."
echo "================================================================================"
# Clear out stale objects and trigger the native kbuild engine loop
rm -f fabriczc_mod.ko src/kernel/*.o 2>/dev/null || true
make clean

# Run the build pass natively against your active Liquorix kernel headers
make

echo "================================================================================"
echo "[+] STEP 3: Verifying Final Binary Artifact Footprint..."
echo "================================================================================"
if [ -f "fabriczc_mod.ko" ]; then
    echo "[SUCCESS] fabriczc_mod.ko successfully compiled via native host kbuild!"
    ls -lh fabriczc_mod.ko
    
    echo "================================================================================"
    echo "[+] STEP 4: Executing Automated Git Synchronization & Push..."
    echo "================================================================================"
    if [ -f "./auto_push_readme.sh" ]; then
        git add Makefile src/kernel/main.c staging_includes/fabriczc_staging.h
        git commit -m "Build Update: Transition sandbox to standard Linux kbuild engine for native host compatibility" || true
        ./auto_push_readme.sh
    fi
else
    echo "[-] Error: Kbuild compilation sequence stalled." >&2
    exit 1
fi

rm -f ./run_host_kbuild.sh 2>/dev/null || true
echo "[+] Host Code Generation and Sync Pipeline Run Completed Cleanly!"
