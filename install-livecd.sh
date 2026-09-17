#!/bin/bash
# Lean, bare-bones rcraid live-CD setup engine.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"

[ "$(id -u)" -eq 0 ] || { echo "Run as root: sudo $0" >&2; exit 1; }

echo "==> [1/3] Compiling and Launching Phase 1 TUI Menu..."
sudo apt-get update -qq && sudo apt-get install -y -qq build-essential "linux-headers-$(uname -r)" dkms pciutils dialog >/dev/null

make -C "$SRC_DIR" clean all >/dev/null

# Execute the compressed modular TUI layout parameters
eval $("$SRC_DIR/scripts/phase1-tui.sh")

# Unbind the generic kernel controller attachments
for disk in $SELECTED_DRIVES; do
    bdf=$(basename "$(readlink -f "/sys/block/$disk/device")")
    [ -e "/sys/bus/pci/drivers/nvme/$bdf" ] && echo "$bdf" > /sys/bus/pci/drivers/nvme/unbind
done

# Load your full-spectrum multi-format optimized driver
insmod "$SRC_DIR/rcraid.ko" enable_writes=1

# Dynamically trigger hardware bus sweeps to build the target node layout
udevadm settle
for _ in {1..10}; do [ -b /dev/rcraid0 ] && break; sleep 0.5; done
[ -b /dev/rcraid0 ] || { echo "❌ /dev/rcraid0 failed to spin up." >&2; exit 1; }

echo "✅ Array active! Open your desktop installer wizard now, target /dev/rcraid0, and install."
read -rp "➡️ Press [Enter] ONLY when the graphical OS installer finishes copying file sectors... " _

echo "==> [2/3] Mapping Mount Bridges and Diving Inside the Target OS..."
# Automatically isolate the main OS rootfs partition slice on the array using blkid lookups
TARGET_PART=$(blkid -t TYPE=ext4 -o device | grep rcraid || blkid -t TYPE=xfs -o device | grep rcraid | head -n1)
TARGET_MNT="/mnt/rcraid-target"

mkdir -p "$TARGET_MNT"
mount "$TARGET_PART" "$TARGET_MNT"

# Direct native recursive system mounts
for sysfs in dev proc sys run; do mount --rbind /$sysfs "$TARGET_MNT/$sysfs" && mount --make-rslave "$TARGET_MNT/$sysfs"; done
mount $(blkid -t TYPE=vfat -o device | grep rcraid | head -n1) "$TARGET_MNT/boot/efi" 2>/dev/null || true
cp -L /etc/resolv.conf "$TARGET_MNT/etc/resolv.conf" 2>/dev/null || true

echo "==> [3/3] Registering DKMS Update Tracking & Baking Initramfs Boot Hooks..."
# Package your core development workspace files straight into the target filesystem tree path
TGT_SRC="$TARGET_MNT/usr/src/rcraid-$(cat VERSION)"
mkdir -p "$TGT_SRC" && cp -r "$SRC_DIR"/{Makefile,dkms.conf,VERSION,rc_*.c,rc_*.h} "$TGT_SRC/"
sed -i "s/@PKGVER@/$(cat VERSION)/g" "$TGT_SRC/dkms.conf"
mkdir -p "$TARGET_MNT/tmp/pkg" && cp -r "$SRC_DIR/packaging" "$TARGET_MNT/tmp/pkg/"

# Execute chroot sequence natively using a compact, direct inline bash sub-shell string block
chroot "$TARGET_MNT" /bin/bash -c "
    apt-get update -qq && apt-get install -y -qq build-essential linux-headers-\$(uname -r) dkms initramfs-tools >/dev/null
    dkms add -m rcraid -v \$(cat /usr/src/rcraid-*/VERSION) --quiet || true
    dkms install -m rcraid -v \$(cat /usr/src/rcraid-*/VERSION) -k \$(uname -r)
    install -m 0755 /tmp/pkg/packaging/sbin/rcraid-bind /usr/sbin/rcraid-bind
    sed 's/@SUBSYSTEM_VENDOR@/0x1022/g' /tmp/pkg/packaging/udev/50-rcraid.rules.in > /etc/udev/rules.d/50-rcraid.rules
    install -m 0755 /tmp/pkg/packaging/initramfs-tools/hooks/rcraid /etc/initramfs-tools/hooks/rcraid
    update-initramfs -u -k all
"

# Advanced fallback UEFI boot entry generation using simple, straight efibootmgr queries
ESP_DEV=$(blkid -t TYPE=vfat -o device | grep rcraid | head -n1 || echo "")
if [ -n "$ESP_DEV" ] && [ -d "$TARGET_MNT/boot/efi/EFI" ]; then
    cp -r "$TARGET_MNT/boot/efi/EFI/"*/* "$TARGET_MNT/boot/efi/EFI/BOOT/BOOTX64.EFI" 2>/dev/null || true
    efibootmgr -c -d "${ESP_DEV%p*}" -p "${ESP_DEV##*p}" -L "rcraid Array" -l "\\EFI\\BOOT\\BOOTX64.EFI" >/dev/null 2>&1 || true
fi

umount -R "$TARGET_MNT" 2>/dev/null || true
echo -e "\n🎉 DONE! The fresh bare-metal PC is 100% prepped. Run 'sudo reboot' to launch your array!"
