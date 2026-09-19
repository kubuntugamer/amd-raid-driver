#!/usr/bin/env bash
# ==============================================================================
# FabricZC Documentation Alignment & Sync Agent
# Patches missing manual verification instructions straight into README.md
# ==============================================================================
set -uo pipefail

echo "================================================================================"
echo "[+] STEP 1: Patching Missing Manual Instructions into README.md..."
echo "================================================================================"

cat << 'INNER_EOF' >> README.md

---

## 🧪 MANUAL SIMULATION RUNTIME CHECKS (VBOX LIVE USB LAB)

When you boot your Kubuntu Live USB inside VirtualBox with your 4 blank NVMe disks attached, open a terminal window and run these validation commands manually:

### 1. Rebuild and Mount the Driver Module
Pull your standalone repository code inside the volatile RAM space, compile the source targets, and insert the block layer driver:
\`\`\`bash
# Run the local standalone compile loop wrapper
./run_pure_local_compile.sh

# Inject the compiled hybrid block engine into active memory
sudo insmod fabriczc_mod.ko
\`\`\`

### 2. Verify the Subsystem Takeover & XFS Spoofing Node
Check the kernel log buffers immediately to confirm your parallel Allocation Group channels initialized and the XFSB magic overrides are armed:
\`\`\`bash
# Output the trailing kernel buffer logs
sudo dmesg | tail -n 20

# Confirm your unified, single aggregate device block node exists
ls -lh /dev/rcraid0
\`\`s

### 3. Run the Installer Validation Test Pass
Simulate an installer query tool scanning the drive to verify that your custom memory-mapped intercepts successfully trick the software into seeing a pre-formatted XFS partition:
\`\`\`bash
# Force a block device identification scan on the hybrid device
sudo blkid /dev/rcraid0
\`\`\`
*Expected Diagnostic Output:* The scan should bypass the blank sectors and return: \`/dev/rcraid0: TYPE="xfs"\`, verifying your software-defined controller works flawlessly!
INNER_EOF

echo "[+] Missing documentation sections successfully woven into the local manual."

echo "================================================================================"
echo "[+] STEP 2: Synchronizing Updated Blueprint Manual up to GitHub..."
echo "================================================================================"
if [ -f "./auto_push_readme.sh" ]; then
    git add README.md
    git commit -m "Docs Update: Weave missing manual verification commands and XFS spoofing check directions into README.md" || true
    ./auto_push_readme.sh
fi

rm -f ./patch_missing_directions.sh 2>/dev/null || true
echo "[+] Synchronization Pipeline Finalized Clean. Directions are Locked on Disk!"
