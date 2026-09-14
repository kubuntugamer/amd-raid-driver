#!/usr/bin/env bash
# ==============================================================================
# TERMINAL BLUEPRINT: NATIVE TREE RESTRUCTURING & SPECIFICATION LINKER
# ROOT REPO MATRIX: /home/chazz/amd-raid-driver
# TARGET LINK NODE: /home/chazz/amd-rcraid
# ==============================================================================

set -e

WORKSPACE_ROOT="/home/chazz/amd-raid-driver"
SPEC_TARGET_DIR="/home/chazz/amd-rcraid"

echo "=== Committing Advanced Folder Structural Sorting Loop ==="

# 1. Verification Guard
if [ ! -f "Makefile" ] || [ ! -f "rc_main.c" ]; then
    echo "[-] Error: Execution context missing required project markers."
    exit 1
fi

# 2. Build Sub-Directory Architecture Layout Trees
mkdir -p src docs build/artifacts

# 3. Restructure Source Code Files into Dedicated src/ Directory
echo "[*] Migrating C/C++ source code elements to src/ folder..."
declare -a CORE_SOURCE_FILES=(
    "rc_main.c" "rc_bottom.c" "rc_config.c" "rc_debugfs.c" 
    "rc_firmware.c" "rc_hw.c" "rc_nvme.c" "rc_sysfs.c"
    "patch_async_worker.c" "patch_parity_math.c" "rc_linux.h" 
    "rc_pci_ids.h" "patch_prototypes.h"
)

for FILE in "${CORE_SOURCE_FILES[@]}"; do
    if [ -f "$FILE" ]; then
        mv "$FILE" src/
    fi
done

# 4. Restructure Markdown Documentation Elements to docs/ Folder
echo "[*] Migrating technical spec documents to docs/ folder..."
declare -a DOC_FILES=(
    "AUDIT.MD" "INSTALL.md" "README.md" "VERSION" "LICENSE"
    "AMD_RAIDCORE_PERSISTENCE_SPEC.md" "AMD_RAIDCORE_MULTISTRIPE_FANOUT.md"
    "AMD_RAIDCORE_NESTED_GEOMETRY.md" "AMD_RAIDCORE_AHCI_ROUTING.md"
    "JOEYTROY_PROJECT_CONTRIBUTION.md" "JOEYTROY_UPSTREAM_ISSUE.md"
    "amd_upstream_proposal.md" "amd_parity_and_worker_proposal.md"
    "SPEC_RAID1_ASYNC.md" "SPEC_RAID56_MATH.md"
)

for DOC in "${DOC_FILES[@]}"; do
    if [ -f "$DOC" ]; then
        mv "$DOC" docs/
    fi
done

# 5. Connect the Symbolic Link Matrix Vector to $HOME/amd-rcraid
echo "[*] Linking specification sheet folder path matrix..."
if [ ! -d "$SPEC_TARGET_DIR" ]; then
    mkdir -p "$SPEC_TARGET_DIR"
fi

if [ -L "linked_specs" ] || [ -f "linked_specs" ]; then
    rm -f "linked_specs"
fi

ln -s "$SPEC_TARGET_DIR" "linked_specs"

# 6. Patch the Makefile to recognize the new src/ directory prefix paths
echo "[*] Updating Makefile inclusion flags for structural mapping..."
if ! grep -q "src/" Makefile; then
    sed -i 's/rc_main.o/src\/rc_main.o/' Makefile
    sed -i 's/rc_bottom.o/src\/rc_bottom.o/' Makefile
    sed -i 's/rc_config.o/src\/rc_config.o/' Makefile
    sed -i 's/rc_debugfs.o/src\/rc_debugfs.o/' Makefile
    sed -i 's/rc_firmware.o/src\/rc_firmware.o/' Makefile
    sed -i 's/rc_hw.o/src\/rc_hw.o/' Makefile
    sed -i 's/rc_nvme.o/src\/rc_nvme.o/' Makefile
    sed -i 's/rc_sysfs.o/src\/rc_sysfs.o/' Makefile
    sed -i 's/patch_async_worker.o/src\/patch_async_worker.o/' Makefile
    sed -i 's/patch_parity_math.o/src\/patch_parity_math.o/' Makefile
fi

echo "======================================================================"
echo "SUCCESS: Workspace structure reorganized and specification linked!"
echo "----------------------------------------------------------------------"
echo " Code Components Folder : $WORKSPACE_ROOT/src/"
echo " Documentation Folder   : $WORKSPACE_ROOT/docs/"
echo " Link Tracking Address  : $WORKSPACE_ROOT/linked_specs -> $SPEC_TARGET_DIR"
echo "======================================================================"
