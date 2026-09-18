#!/bin/bash
# Dedicated Phase 1 AMD RAIDXpert2 Verbose Terminal GUI Component
set -euo pipefail

echo "🔍 [TUI Preflight] Scanning host environment for block-layer rendering dependencies..." >&2
if ! command -v dialog &> /dev/null; then
    echo "📦 [TUI Preflight] 'dialog' binary missing in volatile RAM space. Triggering non-interactive apt-get pool..." >&2
    export DEBIAN_FRONTEND=noninteractive
    sudo apt-get update -qq && sudo apt-get install -y -qq dialog >/dev/null
    echo "✅ [TUI Preflight] Dependency tracking successfully resolved." >&2
else
    echo "✅ [TUI Preflight] Core 'dialog' dependency already active in system tree." >&2
fi

echo "🔍 [TUI Discovery] Sweeping PCIe topology via lsblk for unassigned NVMe endpoints..." >&2
RAW_DISKS=$(lsblk -dno NAME,SIZE | grep nvme | awk '{print $1 " [" $2 "]" " off"}')

if [ -z "$RAW_DISKS" ]; then
    echo "⚠️  [TUI Warning] Zero native NVMe controllers found. Deploying 4x local virtual device mock layers..." >&2
    RAW_DISKS="nvme0n1 [2.0G] off nvme0n2 [2.0G] off nvme0n3 [2.0G] off nvme0n4 [2.0G] off"
fi
echo "📊 [TUI Discovery] Discovered targets available for array indexing:" >&2
echo "$RAW_DISKS" | sed 's/^/    • /' >&2

TEMP_FILE=$(mktemp)
dialog --title " AMD RAIDXpert2 TUI - Select Member Disks " \
       --checklist "Use [Spacebar] to highlight individual disks (MAXIMUM 8 DRIVES):" 15 65 6 \
       $RAW_DISKS 2> "$TEMP_FILE"

SELECTED_DRIVES=$(cat "$TEMP_FILE" | tr -d '"')
rm -f "$TEMP_FILE"

if [ -z "$SELECTED_DRIVES" ]; then
    echo "❌ [TUI Abort] User canceled layout orchestration pass or zero items selected." >&2
    exit 1
fi

DRIVE_ARRAY=($SELECTED_DRIVES)
COUNT=${#DRIVE_ARRAY[@]}
echo "📋 [TUI Filter] Registration pass captured $COUNT targeted components: [ $SELECTED_DRIVES ]" >&2

if [ "$COUNT" -gt 8 ]; then
    dialog --title "⚠️  ARRAY LIMIT EXCEEDED" --msgbox "AMD RAID restricts arrays to a maximum of 8 member disks. You chose $COUNT." 8 65
    echo "❌ [TUI Guard] Aborting setup due to fixed structural limit overrun constraint (Max: 8)." >&2
    exit 1
fi

RAID_LEVEL=$(dialog --title " AMD RAIDXpert2 TUI - Select RAID Level " \
                    --menu "Choose your desired structural array architecture target specification:" 15 65 5 \
                    "0"  "RAID 0 (Stripe - High Performance Optimization)" \
                    "1"  "RAID 1 (Mirror - Redundancy Safeguard)" \
                    "10" "RAID 10 (Striped Mirrors - Minimum 4 Disks Required)" \
                    "5"  "RAID 5 (Distributed Parity - Left Asymmetric Rotation)" \
                    "6"  "RAID 6 (Dual Distributed Parity - Galois Field Multi)" \
                    3>&1 1>&2 2>&3)

echo "⚙️  [TUI Commit] Selected Matrix Level: RAID ${RAID_LEVEL}" >&2
echo "⚙️  [TUI Commit] Packaging environment metrics to persistent exchange node: ${TUI_ENV_FILE}" >&2

# Dump verified properties directly to the parent environment file descriptors
echo "RAID_LEVEL=\"${RAID_LEVEL}\"" > "${TUI_ENV_FILE}"
echo "SELECTED_DRIVES=\"${SELECTED_DRIVES}\"" >> "${TUI_ENV_FILE}"
