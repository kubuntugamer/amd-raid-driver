#!/bin/bash
# Robust, production-ready verbose rcraid modular live-CD setup engine.
set -euo pipefail
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"

[ "$(id -u)" -eq 0 ] || { echo "Run as root: sudo $0" >&2; exit 1; }

. "$SRC_DIR/scripts/lib/secure-boot.sh"

# Secure Boot has to be off for the live session, and that is a property of
# where this script runs rather than an oversight: the live kernel refuses
# unsigned out-of-tree modules, and the only key that could sign this
# session's rcraid.ko would have to be enrolled by rebooting the live session —
# a reboot that throws away the session and the key it just made.  Fail here,
# before anything on disk is touched, instead of at insmod after the user has
# already handed the disks to the installer.
if secure_boot_enforcing; then
    cat >&2 <<'EOF'
ERROR: Secure Boot is enforcing, so this live session cannot load rcraid.ko.

The live kernel rejects unsigned out-of-tree modules ("Key was rejected by
service"), so /dev/rcraid0 never appears and there is nothing to install onto.
No key can fix that from here: enrolling one takes a reboot of the live
session, which is exactly the reboot this installer ends with.

What works:
  1. Switch Secure Boot off in firmware setup.
  2. Run this installer.
  3. Its FIRST boot afterwards enrolls the installed system's own signing key
     in MOK (the installer queues the request; shim's MOK manager asks for the
     password before the kernel starts).  Switch Secure Boot back on once that
     prompt has been answered.

See docs/INSTALL.md, "Secure Boot".
EOF
    exit 1
fi

echo "======================================================================"
echo "🚀 INITIALIZING VERBOSE DRIVER COMPILATION AND INTERFACE HANDOFF"
echo "======================================================================"
echo "📥 Step 1: Gathering live system dependencies..."
sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends -qq build-essential "linux-headers-$(uname -r)" dkms pciutils dialog python3 >/dev/null

echo "🛠️  Step 2: Invoking Makefile tree compilation..."
make -C "$SRC_DIR" clean all

echo "🔍 Step 3: Looking for an array the firmware already created..."
# The array is normally built in the UEFI RAIDXpert2 utility, which writes the
# RAIDCore metadata itself — the same records the driver validates and
# rcraid-bind gates on. When those records are on the disks there is nothing to
# create here: the member list comes from the disks, not from a menu, and
# nothing may be stamped over the firmware's records (which would also destroy
# levels mkmeta.py cannot produce, RAID 5/6 included). The interactive creation
# path is offered only when no disk carries that metadata at all.
RC_MAGIC="52414944436f7265"   # "RAIDCore" at +0x08 of the LBA 0x5000 sector
FW_MEMBERS=""
for disk in $(lsblk -dno NAME,TYPE | awk '$2 == "disk" && $1 ~ /^nvme/ { print $1 }'); do
    magic="$(dd if="/dev/$disk" bs=512 skip=$((0x5000)) count=1 status=none 2>/dev/null \
             | tail -c +9 | head -c 8 | od -An -tx1 -v | tr -d ' \n')"
    if [ "$magic" = "$RC_MAGIC" ]; then
        FW_MEMBERS="$FW_MEMBERS $disk"
    fi
done
FW_MEMBERS="${FW_MEMBERS# }"

if [ -n "$FW_MEMBERS" ]; then
    SELECTED_DRIVES="$FW_MEMBERS"
    RAID_LEVEL="firmware"
    echo "✅ Existing array found — members: ${SELECTED_DRIVES}"
    echo "   Level and geometry come from the RAIDCore metadata already on those"
    echo "   disks; nothing is written to them. Change the array in the UEFI utility."
else
    echo "⚠️  No RAIDCore metadata on any disk, so there is no array to install onto."
    echo "   Arrays are normally created in the UEFI RAIDXpert2 utility, which"
    echo "   writes this metadata. Only continue if you mean to create RAID 0/1 on"
    echo "   blank disks here — that writes fresh metadata over what is on them."
    CREATE_ARRAY="${RCRAID_CREATE_ARRAY:-}"
    if [ -z "$CREATE_ARRAY" ] && [ -t 0 ]; then
        read -rp "Create an array on blank disks instead? [y/N] " _mk
        case "$_mk" in
            [yY]*) CREATE_ARRAY=1 ;;
            *)     CREATE_ARRAY=0 ;;
        esac
    fi
    if [ "$CREATE_ARRAY" != 1 ]; then
        echo "❌ Create the array in the UEFI RAID utility, then re-run this script." >&2
        echo "   (Non-interactive runs: RCRAID_CREATE_ARRAY=1 to create RAID 0/1 here.)" >&2
        exit 1
    fi
    echo "🖥️  Step 3: Launching interactive Terminal GUI wizard configuration pass..."
    TUI_ENV_FILE=$(mktemp)
    export TUI_ENV_FILE
    "$SRC_DIR/scripts/phase1-tui.sh"
    # shellcheck source=/dev/null  # temp file created just above
    . "$TUI_ENV_FILE"
    rm -f "$TUI_ENV_FILE"
fi

clear
echo "======================================================================"
echo "⚙️  EXECUTING PHYSICAL METADATA AND CONTROLLER REGISTRATION PIPELINE"
echo "======================================================================"
if [ "$RAID_LEVEL" = "firmware" ]; then
    echo "📊 Layout                          : from the RAIDCore metadata (RAIDXpert2)"
else
    echo "📊 Targeted Layout Specification   : RAID ${RAID_LEVEL}"
fi
echo "💽 Targeted Disk Components        : ${SELECTED_DRIVES}"
echo "----------------------------------------------------------------------"

if [ "$RAID_LEVEL" = "firmware" ]; then
    echo "📝 Step 1: Using the firmware's RAIDCore records — nothing written to the disks."
else
# Real metadata pass.  scripts/qemu-test/mkmeta.py is the only writer in the
# tree that produces a block rc_nvme_read_validate_metadata() accepts: the
# RC_MetaData sector at LBA 0x5000 (magic "RAIDCore", version 0x00030000 and
# the XOR-lane-shuffle checksum), the config-commit block at 0x5001, and the
# config ring at 0x5800 carrying the LD record plus its element array.  An
# earlier revision stamped 4 bytes (0x25BD + level) here and then printed
# "[Matches: OK]" without comparing anything; no parser can validate that.
echo "📝 Step 1: Writing RAIDCore metadata onto block storage targets..."
case "$RAID_LEVEL" in
    0) MKMETA_LEVEL="raid0" ;;
    1) MKMETA_LEVEL="raid1" ;;
    *) echo "❌ RAID ${RAID_LEVEL} metadata cannot be written from Linux yet." >&2
       echo "   mkmeta.py covers RAID0 and RAID1 only — create RAID ${RAID_LEVEL} in firmware RAIDXpert2." >&2
       exit 1 ;;
esac

MEMBER_PATHS=""
for disk in $SELECTED_DRIVES; do
    if [ -b "/dev/$disk" ]; then
        MEMBER_PATHS="$MEMBER_PATHS /dev/$disk"
    else
        echo "❌ Target path /dev/$disk is not a block device node." >&2
        exit 1
    fi
done

# shellcheck disable=SC2086  # member order IS the on-disk position order
if ! MKMETA_OUT=$(sudo python3 "$SRC_DIR/scripts/qemu-test/mkmeta.py" \
        --level "$MKMETA_LEVEL" $MEMBER_PATHS); then
    echo "❌ mkmeta.py failed — no further changes were made to the disks." >&2
    exit 1
fi
echo "$MKMETA_OUT" | sed 's/^/    • /'

# Verify the write rather than assuming it: read the magic back off every
# member and fail loudly when the bytes are not actually there.
for disk in $SELECTED_DRIVES; do
    # The magic sits at +0x08 of the metadata sector (the 8-byte checksum
    # owns +0x00 — see struct rc_raidcore_md in src/rc_linux.h).
    M_CHECK=$(sudo dd if="/dev/$disk" bs=512 skip=$((0x5000)) count=1 status=none 2>/dev/null \
              | tail -c +9 | head -c 8 | od -An -tx1 -v | tr -d ' \n')
    if [ "$M_CHECK" = "52414944436f7265" ]; then
        echo "    • /dev/$disk: RAIDCore magic verified at LBA 0x5000"
    else
        echo "❌ /dev/$disk: expected RAIDCore magic at LBA 0x5000, read '${M_CHECK:-nothing}'" >&2
        exit 1
    fi
done
fi

echo "🔌 Step 2: Extracting PCIe hardware BDF trees and breaking generic generic driver overrides..."
MEMBER_BDFS=""
for disk in $SELECTED_DRIVES; do
    if [ -e "/sys/block/$disk/device" ]; then
        bdf=$(basename "$(readlink -f "/sys/block/$disk/device")" 2>/dev/null || echo "")
        if [ -n "$bdf" ] && [ -e "/sys/bus/pci/devices/$bdf" ]; then
            MEMBER_BDFS="$MEMBER_BDFS $bdf"
            echo "    • Found PCIe Component: Slot $disk maps to BDF Address $bdf"
            echo "      └─ Detaching native 'nvme' module link from hardware register tree..."
            [ -e "/sys/bus/pci/devices/$bdf/driver_override" ] && echo rcbottom > "/sys/bus/pci/devices/$bdf/driver_override"
            [ -e "/sys/bus/pci/drivers/nvme/$bdf" ] && echo "$bdf" > /sys/bus/pci/drivers/nvme/unbind 2>/dev/null || true
        fi
    fi
done
MEMBER_BDFS="${MEMBER_BDFS# }"

SUBSYSTEM_VENDOR="0x1022"
if [ -n "$MEMBER_BDFS" ]; then
    SUBSYSTEM_VENDOR=$(cat "/sys/bus/pci/devices/$(echo $MEMBER_BDFS | awk '{print $1}')/subsystem_vendor" 2>/dev/null || echo "0x1022")
fi

echo "📥 Step 3: Injecting compiled out-of-tree module (rcraid.ko) with active developer override flags..."
echo "    • Parameters: enable_writes=1 safe_subsys_vendor=${SUBSYSTEM_VENDOR}"
insmod "$SRC_DIR/rcraid.ko" enable_writes=1 "safe_subsys_vendor=$SUBSYSTEM_VENDOR"

if [ -n "$MEMBER_BDFS" ]; then
    echo "⚡ Step 4: Forcing hardware bus probes across unchained PCIe ports..."
    for bdf in $MEMBER_BDFS; do 
        echo "    • Probing PCIe bus address: $bdf"
        echo "$bdf" > /sys/bus/pci/drivers_probe 2>/dev/null || true
    done
fi

echo "🔍 Step 5: Waiting for the driver validation matrix to instantiate /dev/rcraid0..."
udevadm settle 2>/dev/null || true
for i in {1..20}; do
    if [ -b /dev/rcraid0 ]; then
        break
    fi
    echo "    • [Attempt $i/20] Scanning device mapping nodes..."
    sleep 0.5
done

if [ ! -b /dev/rcraid0 ]; then
    echo "❌ Critical Failure: /dev/rcraid0 did not appear." >&2
    echo "   No member was accepted, or the firmware has no array. Check:" >&2
    echo "     dmesg | grep -i 'rcraid\|rc_volume'" >&2
    echo "     lspci -d 1022:b000 -k      # members should show rcbottom" >&2
    exit 1
fi

echo "----------------------------------------------------------------------"
echo "✅ SUCCESS! Unified block storage device array target node is active."
lsblk /dev/rcraid0

# Say plainly what the driver decided about writes. An OS install needs a
# writable volume, and whether this one is writable depends on the level and
# geometry the driver read out of the firmware's records — not on the level
# anyone meant to create.
RO_STATE="$(cat /sys/block/rcraid0/ro 2>/dev/null || echo unknown)"
VOL_LEVEL="$(dmesg 2>/dev/null | grep -o '(level=[A-Za-z0-9]*)' | tail -1 | tr -d '()')"
VOL_LEVEL="${VOL_LEVEL#level=}"
[ -n "$VOL_LEVEL" ] || VOL_LEVEL="unknown"

if [ "$RO_STATE" = "0" ]; then
    echo "✅ Volume is WRITABLE (level ${VOL_LEVEL}) — the OS installer can use it."
else
    echo "❌ Volume is READ-ONLY (level ${VOL_LEVEL}, ro=${RO_STATE}) — an OS install"
    echo "   cannot proceed on it."
    case "$VOL_LEVEL" in
        RAID5|RAID6)
            echo "   This driver has no parity write path (issue #6), so RAID 5/6 volumes"
            echo "   are forced read-only. Recreate the array as RAID 0 or RAID 1 in the"
            echo "   UEFI RAIDXpert2 utility, then re-run this script."
            ;;
        *)
            echo "   The driver refused writes for this level/geometry; dmesg says why:"
            dmesg 2>/dev/null | grep -i "READ-ONLY\|UNTRUSTED" | tail -3 | sed 's/^/     /'
            ;;
    esac
    exit 1
fi
echo "======================================================================"

echo "➡️  Open your graphical Kubuntu installer desktop app now."
echo "➡️  Target /dev/rcraid0 under manual partitioning, map root, and let files copy."
echo "======================================================================"
read -rp "Press [Enter] ONLY when the graphical OS installer finishes copying file sectors... " _

echo "==> [3/3] Mapping Mount Bridges and Diving Inside the Target OS..."
for _part in /dev/rcraid0 /dev/rcraid0p*; do
    [ -b "$_part" ] || continue
    mapfile -t mps < <(findmnt -nro TARGET --source "$_part" 2>/dev/null || true)
    for mp in "${mps[@]}"; do [ -n "$mp" ] && sudo umount -R "$mp" 2>/dev/null || true; done
done

PROBE_DIR=$(mktemp -d)
TARGET_PART=""
target_os=""
for part in /dev/rcraid0p*; do
    [ -b "$part" ] || continue
    if mount -o ro "$part" "$PROBE_DIR" 2>/dev/null; then
        if [ -e "$PROBE_DIR/etc/os-release" ]; then
            TARGET_PART="$part"
            target_os=$(awk -F= '$1 == "ID" {gsub(/["\x27]/, "", $2); print $2; exit}' "$PROBE_DIR/etc/os-release")
            umount "$PROBE_DIR"; break
        fi
        umount "$PROBE_DIR"
    fi
done
rmdir "$PROBE_DIR"

[ -z "$TARGET_PART" ] && { echo "❌ Couldn't find a valid Linux rootfs on the array partitions." >&2; exit 1; }

TARGET_MNT="/mnt/rcraid-target"
mkdir -p "$TARGET_MNT"
mount "$TARGET_PART" "$TARGET_MNT"

for sysfs in dev proc sys run; do mount --rbind /$sysfs "$TARGET_MNT/$sysfs" && mount --make-rslave "$TARGET_MNT/$sysfs"; done

ESP_DEV=""
for part in /dev/rcraid0p*; do
    [ -b "$part" ] || continue
    # Probe the GPT entry directly.  Partitions on the array's own gendisk
    # never get ID_PART_ENTRY_TYPE in the udev database, so `blkid -s PARTTYPE`
    # (and `lsblk -o PARTTYPE`) print nothing for a perfectly valid ESP; they
    # do work for ordinary disks such as the live USB, which is what makes
    # this easy to miss.  `blkid -p` re-reads the parent's GPT itself and
    # reports the type in udev-style output, which is lowercase — lowercase
    # the other side to match.
    part_type=$(blkid -p -o udev "$part" 2>/dev/null | awk -F= '$1 == "ID_PART_ENTRY_TYPE" { print $2; exit }' || true)
    if [ "${part_type,,}" = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b" ]; then
        ESP_DEV="$part"
        mkdir -p "$TARGET_MNT/boot/efi"
        mount "$ESP_DEV" "$TARGET_MNT/boot/efi" 2>/dev/null || true; break
    fi
done
cp -L /etc/resolv.conf "$TARGET_MNT/etc/resolv.conf" 2>/dev/null || true

# The DKMS package version comes from dkms.conf, so the tree staged in the
# target matches the PACKAGE_VERSION dkms is told on the command line there.
TGT_VER="$(sed -n 's/^PACKAGE_VERSION="\([^"]*\)".*/\1/p' "$SRC_DIR/dkms.conf")"
[ -n "$TGT_VER" ] || { echo "❌ Cannot read PACKAGE_VERSION from dkms.conf." >&2; exit 1; }
TGT_SRC="$TARGET_MNT/usr/src/rcraid-${TGT_VER}"
# Same layout as the repo: build files at the root, sources under src/ (the
# root Makefile's rcraid-objs paths are relative to it).
mkdir -p "$TGT_SRC/src"
cp "$SRC_DIR"/Makefile "$SRC_DIR"/dkms.conf "$TGT_SRC/"
for f in "$SRC_DIR"/src/*.c "$SRC_DIR"/src/*.h; do
    case "${f##*/}" in
        test_*) continue ;;
    esac
    cp "$f" "$TGT_SRC/src/"
done
if [ -f "$SRC_DIR/docs/VERSION" ]; then
    cp "$SRC_DIR/docs/VERSION" "$TGT_SRC/VERSION"
fi
# Same build-revision stamp install-dkms.sh writes, so a module built here can
# be matched back to the source it came from (the target tree has no .git).
if _rev=$(git -C "$SRC_DIR" describe --always --dirty 2>/dev/null); then
    echo "$_rev" > "$TGT_SRC/.rcraid_rev"
fi
sed -i "s/@PKGVER@/${TGT_VER}/g" "$TGT_SRC/dkms.conf"

# Ask now, while there is someone to ask: Secure Boot is off at this point (see
# the preflight), so this decides whether the installed system gets its own
# signing key.  Enrolling it is what lets Secure Boot be switched back on
# later, and the enrolment completes at the first boot — the same
# signing-after-install ordering install-dkms.sh uses.
RCRAID_TARGET_MOK="${RCRAID_MOK_ENROLL:-}"
if [ -z "$RCRAID_TARGET_MOK" ]; then
    if [ -t 0 ]; then
        read -rp "Enrol the installed system's module-signing key in MOK (needed to re-enable Secure Boot later)? [Y/n] " _ans
        case "$_ans" in
            [nN]*) RCRAID_TARGET_MOK=0 ;;
            *)     RCRAID_TARGET_MOK=1 ;;
        esac
    else
        RCRAID_TARGET_MOK=1
    fi
fi
if [ "$RCRAID_TARGET_MOK" != 1 ]; then
    RCRAID_TARGET_MOK=0
fi

export RCRAID_TARGET_MOK
mkdir -p "$TARGET_MNT/tmp/pkg"
cp -r "$SRC_DIR/packaging" "$TARGET_MNT/tmp/pkg/"
cp -r "$SRC_DIR/scripts/lib" "$TARGET_MNT/tmp/pkg/"

# The chroot steps live in a script rather than a `bash -c` one-liner so they
# can source the shared signing library and fail loudly: `set -e` inside means
# a failed apt/dkms/initramfs step stops the install instead of being hidden
# behind the exit status of whatever ran last.
cat > "$TARGET_MNT/tmp/pkg/rcraid-target.sh" <<'EOS'
#!/bin/bash
set -euo pipefail

VER="$1"
SUBSYS="$2"
. /tmp/pkg/lib/secure-boot.sh

export DEBIAN_FRONTEND=noninteractive
echo "==> installing build dependencies for the installed kernel(s)"
apt-get update -qq
KVER="$(ls /lib/modules | sort -V | tail -1)"
apt-get install -y -qq build-essential "linux-headers-$KVER" dkms \
    initramfs-tools pciutils efibootmgr mokutil >/dev/null

# --- module signing --------------------------------------------------------
# Configured before the build so the module DKMS installs is signed, with the
# enrolment queued for this system's first boot (shim's MOK manager runs
# before that kernel, so it is trusted the first time it is loaded).
MOK_SIGNING=0
MOK_ENROLL_PENDING=0
MOK_PASSWORD="${RCRAID_MOK_PASSWORD:-}"
MOK_KEY=""
MOK_CERT=""
if [ "${RCRAID_TARGET_MOK:-0}" = 1 ]; then
    if mok_find_enrolled_pair; then
        echo "    • signing key already enrolled: $MOK_KEY"
    elif mok_find_pair || mok_create_pair; then
        echo "    • signing key for this system: $MOK_KEY"
    else
        echo "Could not obtain a module signing key in the target system." >&2
        exit 1
    fi
    if ! mok_write_dkms_conf; then
        echo "DKMS in the target cannot be pointed at $MOK_CERT." >&2
        exit 1
    fi
    MOK_SIGNING=1
fi

# --- build, install, and wire the boot path --------------------------------
# One dkms install per installed kernel: -k all also walks kernels whose
# headers are absent and dies on them, and the live kernel is not one of these.
echo "==> building rcraid $VER with DKMS"
dkms add -m rcraid -v "$VER" --quiet 2>/dev/null || true
for k in /lib/modules/*; do
    [ -e "$k/build" ] || continue
    dkms install -m rcraid -v "$VER" -k "${k##*/}" --force
done

install -m 0755 /tmp/pkg/packaging/sbin/rcraid-bind /usr/sbin/rcraid-bind
sed "s/@SUBSYSTEM_VENDOR@/$SUBSYS/g" \
    /tmp/pkg/packaging/udev/50-rcraid.rules.in > /etc/udev/rules.d/50-rcraid.rules
sed "s/@SUBSYSTEM_VENDOR@/$SUBSYS/g" \
    /tmp/pkg/packaging/modprobe.d/rcraid.conf.in > /etc/modprobe.d/rcraid.conf
install -m 0755 /tmp/pkg/packaging/initramfs-tools/hooks/rcraid \
    /etc/initramfs-tools/hooks/rcraid
# Hold kernel packages out of unattended-upgrades: a kernel update that
# silently broke the DKMS build is an unbootable system when / lives here.
install -m 0644 /tmp/pkg/packaging/apt.conf.d/52-rcraid-kernel-hold \
    /etc/apt/apt.conf.d/52-rcraid-kernel-hold
update-initramfs -u -k all

# Prove the installed copies are signed: when DKMS cannot read its key it warns
# and carries on, and under Secure Boot that only shows up at the next boot, as
# a missing array.
UNSIGNED=0
for ko in /lib/modules/*/updates/dkms/rcraid.ko*; do
    [ -e "$ko" ] || continue
    signer="$(modinfo -F signer "$ko" 2>/dev/null || true)"
    if [ -n "$signer" ]; then
        echo "    • ${ko%/updates/dkms/*}: signed by $signer"
    else
        echo "    • ${ko%/updates/dkms/*}: UNSIGNED" >&2
        UNSIGNED=1
    fi
done
if [ "$UNSIGNED" = 1 ] && [ "$MOK_SIGNING" = 1 ]; then
    echo "ERROR: rcraid was built unsigned even though a signing key was configured." >&2
    exit 1
fi

# Queue the enrolment last: if anything above failed, the user is not left with
# a MOK prompt at the next boot whose password they never saw.
if [ "$MOK_SIGNING" = 1 ] && ! mok_is_enrolled "$MOK_CERT"; then
    MOK_ENROLL_PENDING=1
    if ! mok_queue_enrollment "$MOK_CERT"; then
        echo "Could not queue the MOK enrolment for the target system." >&2
        exit 1
    fi
    echo "    • MOK enrolment queued (password for the next boot: $MOK_PASSWORD)"
fi

{
    echo "MOK_SIGNING=$MOK_SIGNING"
    echo "MOK_ENROLL_PENDING=$MOK_ENROLL_PENDING"
    echo "MOK_CERT=$MOK_CERT"
    echo "MOK_PASSWORD=$MOK_PASSWORD"
} > /tmp/pkg/mok-state
EOS

chroot "$TARGET_MNT" /bin/bash /tmp/pkg/rcraid-target.sh "$TGT_VER" "$SUBSYSTEM_VENDOR"

# Carry the signing outcome back out of the chroot for the closing message.
mok_state_reset
MOK_STATE="$TARGET_MNT/tmp/pkg/mok-state"
mok_load_state "$MOK_STATE"
rm -f "$MOK_STATE"

if [ -n "$ESP_DEV" ] && [ -d "$TARGET_MNT/boot/efi/EFI" ]; then
    mkdir -p "$TARGET_MNT/boot/efi/EFI/BOOT"
    cp "$TARGET_MNT/boot/efi/EFI/$target_os/shimx64.efi" "$TARGET_MNT/boot/efi/EFI/BOOT/BOOTX64.EFI" 2>/dev/null || true
    cp "$TARGET_MNT/boot/efi/EFI/$target_os/grubx64.efi" "$TARGET_MNT/boot/efi/EFI/BOOT/" 2>/dev/null || true
    efibootmgr -c -d "${ESP_DEV%p*}" -p "${ESP_DEV##*p}" -L "rcraid Array Boot" -l "\\EFI\\BOOT\\BOOTX64.EFI" >/dev/null 2>&1 || true
fi

umount -R "$TARGET_MNT" 2>/dev/null || true

mok_enrollment_instructions
if [ "$MOK_SIGNING" = 1 ] && [ "$MOK_ENROLL_PENDING" = 1 ]; then
    echo "After that boot, with the enrolment answered, Secure Boot can be switched"
    echo "back on: the driver and every future DKMS rebuild are signed with it."
    echo
fi

echo -e "\n🎉 DONE! The fresh bare-metal PC is 100% prepped. Run 'sudo reboot' to launch your array!"
