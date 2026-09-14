#!/usr/bin/env bash
# ==============================================================================
# TERMINAL BLUEPRINT: DEVELOPMENT WORKSPACE AUDIT & SPECIFICATION LINKER
# ROOT CORE SOURCE PATH: /home/chazz/amd-raid-driver
# TARGET REPO LINK VECTOR: /home/chazz/amd-rcraid
# ==============================================================================

set -e

WORKSPACE_ROOT="/home/chazz/amd-raid-driver"
SPEC_SOURCE_DIR="/home/chazz/amd-rcraid"
STAGING_POOL="build/artifacts"

echo "=== Committing Workspace Infrastructure Audit & Clean Loop ==="

# 1. Verification Matrix Guard
if [ ! -f "Makefile" ] || [ ! -f "rc_main.c" ]; then
    echo "[-] Error: Execution matrix missing vital core repository layout markers."
    echo "[-] Ensure this terminal shell execution block is run inside the repo path root."
    exit 1
fi

# 2. Build Internal Staging Matrix Enclave
if [ ! -d "$STAGING_POOL" ]; then
    echo "[*] Creating internal compiled binary artifact pool at: $STAGING_POOL"
    mkdir -p "$STAGING_POOL"
fi

# 3. Non-Destructive File Audit Sweep (Moves transient binaries out of root)
echo "[*] Auditing development tree for loose transient build fragments..."
declare -a TRANS_EXTENSIONS=("*.o" "*.mod" "*.mod.c" "*.mod.o" ".*.cmd" "modules.order" "Module.symvers" "rcraid.mod")

for EXT in "${TRANS_EXTENSIONS[@]}"; do
    # Moves loose temporary compile artifacts cleanly into the staging folder
    find . -maxdepth 1 -name "$EXT" -type f -exec mv {} "$STAGING_POOL/" \; 2>/dev/null || true
done

# Keep compiled .ko available via a reference clone loop in the artifacts stack
if [ -f "rcraid.ko" ]; then
    cp rcraid.ko "$STAGING_POOL/"
fi

# 4. Process Cryptographic Symbolic Link Handshake to $HOME/amd-rcraid
echo "[*] Mounting symbolic link paths to core spec sheet directory..."
if [ ! -d "$SPEC_SOURCE_DIR" ]; then
    echo "[*] Target directory '$SPEC_SOURCE_DIR' missing. Initializing core layout path..."
    mkdir -p "$SPEC_SOURCE_DIR"
fi

# Wipe out duplicate or stale link hooks before refreshing the path alignment vector
if [ -L "linked_specs" ] || [ -f "linked_specs" ]; then
    rm -f "linked_specs"
fi

ln -s "$SPEC_SOURCE_DIR" "linked_specs"

# 5. Output Verification Report Matrix
echo "======================================================================"
echo "SUCCESS: Folder audited, structured layout applied, and linked!"
echo "----------------------------------------------------------------------"
echo " Active Workspace Node: $WORKSPACE_ROOT"
echo " Staging Core Enclave : $WORKSPACE_ROOT/$STAGING_POOL"
echo " Symlink Destination  : $WORKSPACE_ROOT/linked_specs -> $SPEC_SOURCE_DIR"
echo "----------------------------------------------------------------------"
echo "Staged Binary Inventory Dump:"
ls -lh "$STAGING_POOL" | awk '{print "   - " $9 " (" $5 ")"}' | grep -v "()$" || true
echo "======================================================================"
