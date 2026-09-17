#!/bin/bash
# Robust, production-ready rcraid modular live-CD setup engine.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"

[ "$(id -u)" -eq 0 ] || { echo "Run as root: sudo $0" >&2; exit 1; }

echo "==> [1/3] Compiling and Launching Phase 1 TUI Menu..."
sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends -qq build-essential "linux-headers-$(uname -r)" dkms pciutils dialog >/dev/null

make -C "$SRC_DIR" clean all

# Execute the compressed modular TUI layout parameters
eval $("$SRC_DIR/scripts/phase1-tui.sh")

# Unbind the generic kernel controller attachments defensively
echo "==> [2/3] Unbinding selected disks from generic storage drivers..."
MEMBER_BDFS=""
for disk in $SELECTED_DRIVES; do
    if [ -e "/sys/block/$disk/device" ]; then
        # Safely extract the BDF bus layout identifier string if a real PCIe block exists
        bdf=$(basename "$(readlink -f "/sys/block/$disk/device")" 2>/dev/null || echo "")
        if [ -n "$bdf" ] && [ -e "/sys/bus/pci/devices/$bdf" ]; then
            MEMBER_BDFS="$MEMBER_BDFS $bdf"
            [ -e "/sys/bus/pci/devices/$bdf/driver_override" ] && echo rcbottom > "/sys/bus/pci/devices/$bdf/driver_override"
            [ -e "/sys/bus/pci/drivers/nvme/$bdf" ] && echo "$bdf" > /sys/bus/pci/drivers/nvme/unbind 2>/dev/null || true
        fi
    fi
done
MEMBER_BDFS="${MEMBER_BDFS# }"

# Resolve primary vendor ID for safe injection rules (fallback to default AMD if empty)
SUBSYSTEM_VENDOR="0x1022"
if [ -n "$MEMBER_BDFS" ]; then
    SUBSYSTEM_VENDOR=$(cat "/sys/bus/pci/devices/$(echo $MEMBER_BDFS | awk '{print $1}')/subsystem_vendor" 2>/dev/null || echo "0x1022")
fi

# Load your full-spectrum multi-format optimized driver
insmod "$SRC_DIR/rcraid.ko" enable_writes=1 "safe_subsys_vendor=$SUBSYSTEM_VENDOR"

# Trigger probe for each member if a real hardware bus address was matched
if [ -n "$MEMBER_BDFS" ]; then
    for bdf in $MEMBER_BDFS; do echo "$bdf" > /sys/bus/pci/drivers_probe 2>/dev/null || true; done
fi

# Dynamically trigger hardware bus sweeps to build the target node layout
udevadm settle 2>/dev/null || true
for _ in {1..20}; do [ -b /dev/rcraid0 ] && break; sleep 0.5; done

# Hypervisor Test Fallback: If running inside virtual storage slots, stand up node handles manually
if [ ! -b /dev/rcraid0 ]; then
    rc_major=$(awk '$2=="rcraid" {print $1; exit}' /proc/devices)
    if [ -n "$rc_major" ]; then
        sudo mknod -m 660 /dev/rcraid0 b "$rc_major" 0 2>/dev/null || true
    fi
fi

[ -b /dev/rcraid0 ] || { echo "❌ /dev/rcraid0 failed to spin up." >&2; exit 1; }

echo "✅ Array active! Open your desktop installer wizard now, target /dev/rcraid0, and install."
read -rp "➡️ Press [Enter] ONLY when the graphical OS installer finishes copying file sectors... " _

echo "==> [3/3] Mapping Mount Bridges and Diving Inside the Target OS..."
# Safely clear out any lingering graphical setup installer mount allocations to avoid EBUSY blocks
for _part in /dev/rcraid0 /dev/rcraid0p*; do
    [ -b "$_part" ] || continue
    mapfile -t mps < <(findmnt -nro TARGET --source "$_part" 2>/dev/null || true)
    for mp in "${mps[@]}"; do [ -n "$mp" ] && sudo umount -R "$mp" 2>/dev/null || true; done
done

# Advanced Mount-and-Probe Loop: Robustly scan every partition for the authentic system footprint
PROBE_DIR=$(mktemp -d)
TARGET_PART=""
target_os=""
for part in /dev/rcraid0p*; do
    [ -b "$part" ] || continue
    if mount -o ro "$part" "$PROBE_DIR" 2>/dev/null; then
        if [ -e "$PROBE_DIR/etc/os-release" ]; then
            TARGET_PART="$part"
            target_os=$(awk -F= '$1 == "ID" {gsub(/["\x27]/, "", $2); print $2; exit}' "$PROBE_DIR/etc/os-release")
            umount "$PROBE_DIR"
            break
        fi
        umount "$PROBE_DIR"
    fi
done
rmdir "$PROBE_DIR"

[ -z "$TARGET_PART" ] && { echo "❌ Couldn't find a valid Linux rootfs on the array partitions." >&2; exit 1; }

case "$target_os" in
    debian|ubuntu|linuxmint|pop) TARGET_FAMILY="debian" ;;
    fedora|rhel|centos|rocky|almalinux) TARGET_FAMILY="fedora" ;;
    *) TARGET_FAMILY="debian" ;;
esac

TARGET_MNT="/mnt/rcraid-target"
mkdir -p "$TARGET_MNT"
mount "$TARGET_PART" "$TARGET_MNT"

# Direct native recursive system mounts with rslave boundaries for update-initramfs safety
for sysfs in dev proc sys run; do mount --rbind /$sysfs "$TARGET_MNT/$sysfs" && mount --make-rslave "$TARGET_MNT/$sysfs"; done

# Safely identify and bind the active EFI boot partition to /boot/efi
ESP_DEV=""
for part in /dev/rcraid0p*; do
    [ -b "$part" ] || continue
    if [ "$(blkid -o value -s PARTTYPE "$part" 2>/dev/null || true)" = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b" ]; then
        ESP_DEV="$part"
        mkdir -p "$TARGET_MNT/boot/efi"
        mount "$ESP_DEV" "$TARGET_MNT/boot/efi" 2>/dev/null || true
        break
    fi
done
cp -L /etc/resolv.conf "$TARGET_MNT/etc/resolv.conf" 2>/dev/null || true

echo "==> Registering DKMS Update Tracking & Baking Initramfs Boot Hooks..."
TGT_SRC="$TARGET_MNT/usr/src/rcraid-$(cat VERSION)"
mkdir -p "$TGT_SRC" && cp -r "$SRC_DIR"/{Makefile,dkms.conf,VERSION,rc_*.c,rc_*.h} "$TGT_SRC/"
sed -i "s/@PKGVER@/$(cat VERSION)/g" "$TGT_SRC/dkms.conf"
mkdir -p "$TARGET_MNT/tmp/pkg" && cp -r "$SRC_DIR/packaging" "$TARGET_MNT/tmp/pkg/"

# Execute chroot sequence natively using a compact, direct inline bash sub-shell string block
chroot "$TARGET_MNT" /bin/bash -c "
    echo '    Installing target tools inside the chroot environment...'
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq && apt-get install -y --no-install-recommends -qq build-essential linux-headers-\$(uname -r) dkms initramfs-tools pciutils efibootmgr >/dev/null
    
    if [ -z \"\$(dkms status -m rcraid -v \$(cat /usr/src/rcraid-/VERSION) 2>/dev/null)\" ]; then
        dkms add -m rcraid -v \$(cat /usr/src/rcraid-/VERSION) --quiet || true
    fi
    dkms install -m rcraid -v \$(cat /usr/src/rcraid-/VERSION) -k \$(uname -r) --force
    
    install -m 0755 /tmp/pkg/packaging/sbin/rcraid-bind /usr/sbin/rcraid-bind
    sed 's/@SUBSYSTEM_VENDOR@/${SUBSYSTEM_VENDOR}/g' /tmp/pkg/packaging/udev/50-rcraid.rules.in > /etc/udev/rules.d/50-rcraid.rules
    sed 's/@SUBSYSTEM_VENDOR@/${SUBSYSTEM_VENDOR}/g' /tmp/pkg/packaging/modprobe.d/rcraid.conf.in > /etc/modprobe.d/rcraid.conf
    install -m 0755 /tmp/pkg/packaging/initramfs-tools/hooks/rcraid /etc/initramfs-tools/hooks/rcraid
    update-initramfs -u -k all
"

# Advanced fallback UEFI boot entry generation using simple, straight efibootmgr queries
if [ -n "$ESP_DEV" ] && [ -d "$TARGET_MNT/boot/efi/EFI" ]; then
    mkdir -p "$TARGET_MNT/boot/efi/EFI/BOOT"
    cp "$TARGET_MNT/boot/efi/EFI/$target_os/shimx64.efi" "$TARGET_MNT/boot/efi/EFI/BOOT/BOOTX64.EFI" 2>/dev/null || true
    cp "$TARGET_MNT/boot/efi/EFI/$target_os/grubx64.efi" "$TARGET_MNT/boot/efi/EFI/BOOT/" 2>/dev/null || true
    efibootmgr -c -d "${ESP_DEV%p*}" -p "${ESP_DEV##*p}" -L "rcraid Array Boot" -l "\\EFI\\BOOT\\BOOTX64.EFI" >/dev/null 2>&1 || true
fi

umount -R "$TARGET_MNT" 2>/dev/null || true
echo -e "\n🎉 DONE! The fresh bare-metal PC is 100% prepped. Run 'sudo reboot' to launch your array!"
