#!/bin/bash
# Dedicated Phase 1 AMD RAIDXpert2 Terminal GUI Component (Max 8 Disks Guarded)
set -euo pipefail

# Ensure lightweight dialog rendering libraries are available
command -v dialog &>/dev/null || { sudo apt-get update -qq && sudo apt-get install -y -qq dialog >/dev/null; }

# Scan for all raw, unassigned physical NVMe drives available on the PCIe bus
RAW_DISKS=$(lsblk -dno NAME,SIZE | grep nvme | awk '{print $1 " [" $2 "]" " off"}')

if [ -z "$RAW_DISKS" ]; then
    dialog --title "⚠️  DRIVE ERROR" --msgbox "No physical NVMe drives discovered on the PCIe bus architecture. Ensure slots are active." 8 65
    exit 1
fi

while :; do
    # Render interactive checklist menu for disk selection
    TEMP_FILE=$(mktemp)
    dialog --title " AMD RAIDXpert2 TUI - Select Member Disks " \
           --checklist "Use [Spacebar] to highlight individual disks (MAXIMUM 8 DRIVES):" 16 65 6 \
           $RAW_DISKS 2> "$TEMP_FILE"

    SELECTED_DRIVES=$(cat "$TEMP_FILE" | tr -d '"')
    rm -f "$TEMP_FILE"

    if [ -z "$SELECTED_DRIVES" ]; then
        clear
        echo "❌ Array configuration workflow canceled: No hardware disks selected."
        exit 1
    fi

    # CRITICAL SECURITY GUARD: Count the exact number of selected drives
    # Converts the plain string list into an array to run an absolute length check
    DRIVE_ARRAY=($SELECTED_DRIVES)
    COUNT=${#DRIVE_ARRAY[@]}

    if [ "$COUNT" -gt 8 ]; then
        dialog --title "⚠️  ARRAY LIMIT EXCEEDED" \
               --msgbox "AMD RAIDXpert2 firmware restricts arrays to a maximum of 8 member disks.\n\nYou selected $COUNT drives. Please scale down your choices." 10 65
        continue # Loop right back to the checklist window without advancing
    fi
    
    break # Configuration is within safe hardware parameters; exit the loop safely
done

# Render choice grid layout for array format level selection
RAID_LEVEL=$(dialog --title " AMD RAIDXpert2 TUI - Select RAID Level " \
                    --menu "Choose your desired structural array architecture target specification:" 15 65 5 \
                    "0"  "RAID 0 (Stripe - High Performance Optimization)" \
                    "1"  "RAID 1 (Mirror - Redundancy Safeguard)" \
                    "10" "RAID 10 (Striped Mirrors - Minimum 4 Disks Required)" \
                    "5"  "RAID 5 (Distributed Parity - Left Asymmetric Rotation)" \
                    "6"  "RAID 6 (Dual Distributed Parity - Galois Field Multi)" \
                    3>&1 1>&2 2>&3)

# Export verified, safe selections back to parent shell execution environment
echo "RAID_LEVEL=\"${RAID_LEVEL}\""
echo "SELECTED_DRIVES=\"${SELECTED_DRIVES}\""
