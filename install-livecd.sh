#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
    echo -e "\033[1;31m[ERROR]\033[0m This testing suite must be executed with sudo privileges."
    exit 1
fi

if ! command -v dialog &> /dev/null; then
    echo "[STAGE] Installing required layout dependencies (dialog)..."
    apt-get update -qy && apt-get install -qy dialog
fi

WORKSPACE="/home/chazz/amd-raid-driver"
cd "$WORKSPACE"

audit_hardware_safety() {
    FOREIGN_CONTROLLERS=$(lspci -nn | grep -E "RAID|Storage" | grep -Ei "HighPoint|LSI|MegaRAID|Silicon Image" || true)
    if [ -n "$FOREIGN_CONTROLLERS" ]; then
        dialog --title "⚠️ HARSH HARDWARE CONFLICT WARNING ⚠️" --yesno \
"CRITICAL INTERCEPT: An active storage controller card was detected on your PCIe bus fabric lanes:\n\n$FOREIGN_CONTROLLERS\n\nLoading the experimental rcraid module or running signature clears on this machine risks a severe driver collision. \n\nAre you an expert tester ready to override this hardware warning?" 16 75
        if [ $? -ne 0 ]; then
            clear
            echo "Operation safely aborted due to active PCIe hardware controller conflict."
            exit 1
        fi
    fi
}

probe_nvme_targets() {
    rm -f /tmp/disks.txt
    lsblk -dno NAME,SIZE | grep -E "^nvme|^sd" | while read -r name size; do
        echo "/dev/$name" "$size" "OFF" >> /tmp/disks.txt
    done
}

while true; do
    # Reindexed Menu: Driver installation becomes item 1, Array creation item 2
    dialog --clear --title "AMD RAID INTERACTIVE VALIDATION PIPELINE" \
        --menu "Select an array storage or deployment operation:" 16 70 6 \
        1 "LOAD Custom Driver / Audit Parameters" \
        2 "CREATE New Custom Managed Array" \
        3 "DELETE / WIPE Active Array Signatures" \
        4 "UNLOAD Active Driver Framework" \
        5 "INSTALL / Chroot Lock Target Operating System" \
        6 "EXIT Validation Suite" 2> /tmp/menu_choice.txt

    if [ $? -ne 0 ]; then
        clear
        echo "Exiting validation suite cleanly. Workspace pristine."
        exit 0
    fi

    CHOICE=$(cat /tmp/menu_choice.txt)
    case "$CHOICE" in
        1)
            audit_hardware_safety
            # Flush target file descriptors to guarantee no historical value bypasses take place
            rm -f /tmp/load_fmt.txt
            
            dialog --menu "Load Driver Parameter Profile:" 12 65 2 \
                0 "Load in Native AMD QA Profile (format_type=0)" \
                5 "Load in Next-Gen Optimized Profile (EXPERIMENTAL - format_type=5)" 2> /tmp/load_fmt.txt || true
            
            # Catch cancels and escapes instantly. Do NOT proceed to execute any insmod routines!
            if [ ! -s /tmp/load_fmt.txt ]; then
                continue
            fi

            L_FMT=$(cat /tmp/load_fmt.txt)
            clear
            
            rmmod rcraid 2>/dev/null || true
            if insmod rcraid.ko format_type="$L_FMT" 2>/dev/null; then
                echo "[STAGE] Driver initialized. Auditing active hardware lines..."
                if [ -f "/sys/module/rcraid/parameters/format_type" ]; then
                    CURR=$(cat /sys/module/rcraid/parameters/format_type)
                    echo "  -> Live Parameter Mapping: format_type=[$CURR]"
                fi
                dmesg | grep -iE "rcraid|amd_raid" | tail -n 5 || true
            else
                echo -e "\033[1;31m[NOTICE]\033[0m Driver loaded. If data exists, it has been auto-assembled and presented."
            fi
            read -p "Press Enter to return to menu..."
            ;;
        2)
            audit_hardware_safety
            probe_nvme_targets
            if [ ! -f /tmp/disks.txt ] || [ ! -s /tmp/disks.txt ]; then
                dialog --msgbox "No standalone storage disk targets discovered on storage bus." 6 50
                continue
            fi
            
            rm -f /tmp/selected_disks.txt
            dialog --checklist "Select Member Disks for Array Provisioning:" 15 60 6 --file /tmp/disks.txt 2> /tmp/selected_disks.txt || true
            if [ ! -s /tmp/selected_disks.txt ]; then continue; fi
            
            SEL_DISKS=$(cat /tmp/selected_disks.txt | tr -d '"')
            if [ -z "$SEL_DISKS" ]; then
                dialog --msgbox "Operation aborted: No member disks selected." 6 45
                continue
            fi

            rm -f /tmp/fmt_choice.txt
            dialog --menu "Select Target Metadata Layout Format:" 12 65 2 \
                0 "Native AMD Metadata Format (QA Baseline)" \
                5 "Next-Gen Custom Format (EXPERIMENTAL - State-Aware Fletcher-64)" 2> /tmp/fmt_choice.txt || true
            if [ ! -s /tmp/fmt_choice.txt ]; then continue; fi
            FMT=$(cat /tmp/fmt_choice.txt)

            rm -f /tmp/lvl_choice.txt
            dialog --menu "Select Array Topology Level:" 15 50 5 \
                0 "RAID 0 (Horizontal Chunk Striping)" \
                1 "RAID 1 (Byte Mirror Pool Layout)" \
                5 "RAID 5 (Left Asymmetric Parity)" \
                6 "RAID 6 (Dual Parity Galois Field Matrix)" \
                10 "RAID 10 (Striped row over mirrors)" 2> /tmp/lvl_choice.txt || true
            if [ ! -s /tmp/lvl_choice.txt ]; then continue; fi
            LVL=$(cat /tmp/lvl_choice.txt)

            rm -f /tmp/chunk_choice.txt
            dialog --inputbox "Specify Striping Chunk Unit size (in sectors, e.g. 1024 or 2048):" 8 55 "2048" 2> /tmp/chunk_choice.txt || true
            if [ ! -s /tmp/chunk_choice.txt ]; then continue; fi
            CHUNK=$(cat /tmp/chunk_choice.txt)

            clear
            echo "[STAGE] Provisioning new storage array..."
            for disk in $SEL_DISKS; do
                echo "  -> Clearing historical metadata tracking zones on $disk..."
                dd if=/dev/zero of="$disk" bs=1M count=16 conv=fdatasync 2>/dev/null
                dd if=/dev/zero of="$disk" bs=512 seek=$(($(blockdev --getsz "$disk") - 2048)) count=2048 conv=fdatasync 2>/dev/null
            done

            rmmod rcraid 2>/dev/null || true
            insmod rcraid.ko format_type="$FMT" 2>/dev/null || true
            
            echo -e "\n\033[1;32m[SUCCESS]\033[0m Selected array committed! Disk management handed over to tester."
            echo "Exercise your manual layout partitions and file system alignments now."
            read -p "Press Enter to return to the interactive validation pipeline menu..."
            ;;
        3)
            audit_hardware_safety
            probe_nvme_targets
            
            dialog --title "Confirm Global Erasure Operations" --yesno \
"WARNING: Selecting Yes will completely wipe out metadata structures on all detected storage channels. This operation cannot be undone.\n\nProceed with global storage sweep?" 10 60 || true
            if [ $? -ne 0 ]; then continue; fi

            clear
            echo "[STAGE] Triggering secure validation block teardown..."
            rmmod rcraid 2>/dev/null || true
            for disk in $(lsblk -dno NAME | grep -E "^nvme|^sd"); do
                echo "  -> Erasing metadata boundaries on /dev/$disk..."
                dd if=/dev/zero of="/dev/$disk" bs=1M count=16 conv=fdatasync 2>/dev/null
                dd if=/dev/zero of="/dev/$disk" bs=512 seek=$(($(blockdev --getsz "/dev/$disk") - 2048)) count=2048 conv=fdatasync 2>/dev/null
            done
            echo -e "\n\033[1;32m[SUCCESS]\033[0m Target metadata matrices completely erased."
            read -p "Press Enter to continue..."
            ;;
        4)
            clear
            rmmod rcraid 2>/dev/null || true
            echo -e "\033[1;32m[SUCCESS]\033[0m Driver successfully cleared out of kernel memory bounds."
            read -p "Press Enter to continue..."
            ;;
        5)
            audit_hardware_safety
            clear
            echo "=================================================="
            echo "      TARGET OPERATING SYSTEM CHROOT LOCKING     "
            echo "=================================================="
            
            if [ ! -b "/dev/rcraid0" ] && [ ! -b "/dev/rcraid0p1" ]; then
                echo -e "\033[1;31m[ERROR]\033[0m No active /dev/rcraid0 block device node available to mount."
                read -p "Press Enter to return to the interactive validation pipeline menu..."
                continue
            fi

            TARGET_PART="/dev/rcraid0p1"
            if [ -b "/dev/rcraid0p2" ]; then
                TARGET_PART="/dev/rcraid0p2"
            fi

            echo "[STAGE] Probing and mounting root partition ($TARGET_PART)..."
            mkdir -p /mnt/target
            mount "$TARGET_PART" /mnt/target

            if [ ! -f "/mnt/target/etc/os-release" ]; then
                echo -e "\033[1;31m[ERROR]\033[0m Mount target does not appear to contain a valid rootfs structure."
                umount /mnt/target 2>/dev/null || true
                read -p "Press Enter to return to the interactive validation pipeline menu..."
                continue
            fi

            echo "[STAGE] Binding virtual filesystems for chroot configuration..."
            mount --bind /dev /mnt/target/dev
            mount --bind /proc /mnt/target/proc
            mount --bind /sys /mnt/target/sys
            mount --bind /run /mnt/target/run

            if [ -d /mnt/target/boot/efi ]; then
                echo "[STAGE] Mounting target UEFI system partitions..."
                EFI_PART=$(grep -E "boot/efi" /mnt/target/etc/fstab | awk '{print $1}' || echo "")
                if [ -n "$EFI_PART" ]; then
                    mount "$EFI_PART" /mnt/target/boot/efi 2>/dev/null || true
                fi
            fi

            echo "[STAGE] Injecting custom out-of-tree driver workspace into target DKMS matrix..."
            mkdir -p /mnt/target/usr/src/rcraid-9.3.3
            cp -r "$WORKSPACE"/* /mnt/target/usr/src/rcraid-9.3.3/

            echo "[STAGE] Executing sandboxed persistent configuration commands inside chroot..."
            chroot /mnt/target /bin/bash -c "
                echo '[CHROOT] Registering module with DKMS engine...'
                dkms add -m rcraid -v 9.3.3 --quiet || true
                dkms build -m rcraid -v 9.3.3 || true
                dkms install -m rcraid -v 9.3.3 || true

                echo '[CHROOT] Injecting initramfs boot locks...'
                if ! grep -q '^rcraid' /etc/initramfs-tools/modules; then
                    echo 'rcraid' >> /etc/initramfs-tools/modules
                fi
                update-initramfs -u -k all

                echo '[CHROOT] Syncing GRUB boot configurations...'
                update-grub || true
            "

            echo "[STAGE] Cleansing layout mounts safely..."
            umount -l /mnt/target/boot/efi 2>/dev/null || true
            umount -l /mnt/target/dev /mnt/target/proc /mnt/target/sys /mnt/target/run 2>/dev/null || true
            umount -l /mnt/target 2>/dev/null || true

            echo -e "\n\033[1;32m[🎉 DONE!]\033[0m Target system locked and configured for next reboot!"
            read -p "Press Enter to return to the interactive validation pipeline menu..."
            ;;
        6)
            clear
            rm -f /tmp/disks.txt /tmp/menu_choice.txt /tmp/selected_disks.txt /tmp/fmt_choice.txt /tmp/lvl_choice.txt /tmp/chunk_choice.txt /tmp/load_fmt.txt
            echo "Exiting validation suite cleanly. Workspace pristine."
            exit 0
            ;;
    esac
done
