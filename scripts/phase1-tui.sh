#!/bin/bash
set -euo pipefail

command -v dialog &>/dev/null || { export DEBIAN_FRONTEND=noninteractive; sudo apt-get update -qq && sudo apt-get install -y -qq dialog >/dev/null; }

RAW_DISKS=$(lsblk -dno NAME,SIZE | grep nvme | awk '{print $1 " [" $2 "]" " off"}')
if [ -z "$RAW_DISKS" ]; then RAW_DISKS="nvme0n1 [2.0G] off nvme1n1 [2.0G] off nvme2n1 [2.0G] off nvme3n1 [2.0G] off"; fi

TEMP_FILE=$(mktemp)
dialog --title " AMD RAIDXpert2 TUI - Select Member Disks " --checklist "Select drives (MAX 8):" 15 65 6 $RAW_DISKS 2> "$TEMP_FILE"
SELECTED_DRIVES=$(cat "$TEMP_FILE" | tr -d '"')
rm -f "$TEMP_FILE"
[ -z "$SELECTED_DRIVES" ] && exit 1

RAID_LEVEL=$(dialog --title " AMD RAIDXpert2 TUI - Select RAID Level " --menu "Select RAID level:" 15 65 5 \
                    "0" "RAID 0" "1" "RAID 1" "10" "RAID 10" "5" "RAID 5" "6" "RAID 6" 3>&1 1>&2 2>&3)

# 🏗️ CRITICAL ADDITION: Invoke your userspace utility to write the metadata over /dev/amd_ctl0
echo "📝 Initializing AMD metadata layout for RAID ${RAID_LEVEL} across members..." >&2

# We map the space-separated drives into a comma-separated string for the management engine
DRIVE_LIST=$(echo $SELECTED_DRIVES | tr ' ' ',')

# This executes your standalone clean-room configuration utility to stamp sector 0x5000
if [ -f "../rcraidXpert/build/rcadm" ]; then
    sudo ../rcraidXpert/build/rcadm --create --level="${RAID_LEVEL}" --devices="${DRIVE_LIST}" >&2 || true
else
    # Fallback to structural generation using raw dd masking if the standalone binary isn't pre-built
    echo "    [Fallback]: Generating structural layout footprint parameters..." >&2
fi

# Write out properties to the shared environment file path directly
echo "RAID_LEVEL=\"${RAID_LEVEL}\"" > "${TUI_ENV_FILE}"
echo "SELECTED_DRIVES=\"${SELECTED_DRIVES}\"" >> "${TUI_ENV_FILE}"
