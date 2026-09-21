#!/bin/bash
# FabricZC Alpha Production Installer: High-Speed Live-CD Integration Script
TARGET_ROOT="/mnt/rcraid_root"

clear
echo "======================================================================"
echo "          FABRICZC BLOCK DRIVER: PHASE 1 ARRAY INITIALIZATION         "
echo "======================================================================"

# A. Safeguard directory paths and remove running driver artifacts cleanly
umount -lf "$TARGET_ROOT/boot/efi" 2>/dev/null || true
umount -lf "$TARGET_ROOT" 2>/dev/null || true
if lsmod | grep -q "fabriczc_mod"; then rmmod fabriczc_mod 2>/dev/null || true; fi
sleep 1

# B. Inject the fresh, dynamically scaling storage driver module into memory
echo "[*] Activating FabricZC storage translation layer..."
cd "$(dirname "$0")" || exit 1
insmod fabriczc_mod.ko
sleep 2

# C. Create aligned partition layouts strictly over the virtual device node track
echo "[*] Generating fresh partition topologies on /dev/rcraid0..."
parted -s /dev/rcraid0 mklabel gpt \
    mkpart primary fat32 2048s 1050623s \
    mkpart primary xfs 1050624s 100%

echo "[*] Laying down native file systems (VFAT + XFS)..."
mkfs.vfat -F 32 /dev/rcraid0p1 >/dev/null
mkfs.xfs -f -K /dev/rcraid0p2 >/dev/null

echo "======================================================================"
echo "[+] PHASE 1 COMPLETE: /dev/rcraid0 is initialized and ready!"
echo "----------------------------------------------------------------------"
echo "  👉 ACTION REQUIRED: Minimize this terminal window now."
echo "  👉 Launch your graphical Live CD desktop OS Installer wizard."
echo "  👉 Select 'Manual Partitioning' and map files over /dev/rcraid0p2."
echo "  👉 Map your boot EFI target loader over /dev/rcraid0p1."
echo "  👉 Complete the wizard copy loops. DO NOT REBOOT WHEN FINISHED!"
echo "  👉 Close the installer window, return here, and hit Enter."
echo "======================================================================"
read -p "Press [Enter] after the graphical OS copy loop concludes to fire Stage 2..."

echo "======================================================================"
echo "          FABRICZC BLOCK DRIVER: PHASE 2 CHROOT LOCKDOWN              "
echo "======================================================================"

# D. Re-mount the freshly populated target filesystem tracks cleanly
echo "[*] Securing execution mount interfaces..."
mount -o rw,exec,dev,noatime,nodiratime /dev/rcraid0p2 "$TARGET_ROOT"
mkdir -p "$TARGET_ROOT/boot/efi"
mount -o rw,exec,dev /dev/rcraid0p1 "$TARGET_ROOT/boot/efi"

# E. Bridge runtime system API namespaces into the new target rootfs tree
VIRT_SUBSYSTEMS=("dev" "dev/pts" "proc" "sys" "run")
for sub in "${VIRT_SUBSYSTEMS[@]}"; do
    if ! mountpoint -q "$TARGET_ROOT/$sub"; then
        mount --bind "/$sub" "$TARGET_ROOT/$sub"
    fi
done

# Copy network profiles and the stable driver module binary into the target directory paths
cp -L /etc/resolv.conf "$TARGET_ROOT/etc/resolv.conf" 2>/dev/null || true
mkdir -p "$TARGET_ROOT/lib/modules/7.0.0-14-generic/kernel/drivers/block"
cp fabriczc_mod.ko "$TARGET_ROOT/lib/modules/7.0.0-14-generic/kernel/drivers/block/"

# F. Copy your standalone Stage 2 script straight into the target filesystem and trigger execution
echo "[*] Transitioning system control to Stage 2 automated finalizer script..."
cp src/scripts/finalize_alpha_boot.sh "$TARGET_ROOT/tmp/"
chmod +x "$TARGET_ROOT/tmp/finalize_alpha_boot.sh"

chroot "$TARGET_ROOT" /tmp/finalize_alpha_boot.sh

# G. Clean up the finalizer asset and unhook the virtual system bindings
rm -f "$TARGET_ROOT/tmp/finalize_alpha_boot.sh"
sync

echo "[*] Safely dismantling virtual bind structures..."
umount -lf "$TARGET_ROOT/dev/pts" 2>/dev/null || true
umount -lf "$TARGET_ROOT/dev" 2>/dev/null || true
umount -lf "$TARGET_ROOT/proc" 2>/dev/null || true
umount -lf "$TARGET_ROOT/sys" 2>/dev/null || true
umount -lf "$TARGET_ROOT/run" 2>/dev/null || true
umount -lf "$TARGET_ROOT/boot/efi" 2>/dev/null || true
umount -lf "$TARGET_ROOT" 2>/dev/null || true
sync

echo "======================================================================"
echo "[🎉] ALPHA DEPLOYMENT SUCCESSFUL! Bootloader is sealed on the container tracks."
echo "[🎉] Run 'sudo reboot' now to boot your fresh installation!"
echo "======================================================================"
