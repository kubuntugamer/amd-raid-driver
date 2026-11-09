#!/bin/bash
# Install rcraid via DKMS plus boot-time auto-bind glue.
#
# After this script succeeds, the array survives reboots:
#   - DKMS rebuilds the module against every kernel update.
#   - A udev rule hands matching PCI devices to rcbottom at hot-plug time,
#     so /dev/rcraid0 appears during boot without any manual unbind+insmod.
#   - A modprobe.d drop-in records `enable_writes=1` and the array's
#     `safe_subsys_vendor`, so no extra args are needed at load time.
#
# Requires root.  Run from the repo root.

set -euo pipefail

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
PKG_NAME="rcraid"
# The dkms package version tracks the driver version (VERSION file); the
# repo dkms.conf carries a @PKGVER@ placeholder we substitute at staging
# time, so each driver release installs into its own dkms tree.
DRIVER_VERSION="$(cat "$SRC_DIR/VERSION")"
PKG_VERSION="$DRIVER_VERSION"
DKMS_SRC="/usr/src/${PKG_NAME}-${PKG_VERSION}"

[ "$(id -u)" -eq 0 ] || { echo "must be root" >&2; exit 1; }

# Tooling preflight — fail fast with an actionable message rather than later
# with a silent "no devices found" or a confusing dkms error.
for tool in lspci dkms udevadm; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        case "$tool" in
            lspci)   pkg="pciutils" ;;
            dkms)    pkg="dkms" ;;
            udevadm) pkg="systemd / udev" ;;
        esac
        echo "missing required tool: $tool (install $pkg)" >&2
        exit 1
    fi
done

# Secure Boot / module signing preflight.
#
# An enforcing kernel refuses an unsigned module ("Key was rejected by
# service"), and the failure surfaces only at the next boot, with the array
# absent.  DKMS signs every module it builds (/usr/sbin/dkms prepare_signing)
# once it knows which key to use, so the work here is:
#   1. find a key pair that is BOTH on disk and enrolled in MOK,
#   2. point DKMS at it, including for every future kernel update,
#   3. verify after the build that the installed module really is signed.
# Step 3 is what keeps kernel updates bootable: a rebuild that silently lost
# the key must fail here, not on an unbootable machine.
. "$SRC_DIR/scripts/lib/secure-boot.sh"

# Not `func && VAR=1`: under `set -e` a false return would abort the script.
SB_ENFORCING=0
if secure_boot_enforcing; then
    SB_ENFORCING=1
fi

# Resolve the key pair.  Order: explicit override, then the shim key that
# update-secureboot-policy creates on Debian/Ubuntu, then DKMS's own default.
MOK_KEY="${RCRAID_MOK_KEY:-}"
MOK_CERT="${RCRAID_MOK_CERT:-}"
if [ -z "$MOK_KEY" ] || [ -z "$MOK_CERT" ]; then
    for pair in "/var/lib/shim-signed/mok/MOK.priv:/var/lib/shim-signed/mok/MOK.der" \
                "/var/lib/dkms/mok.key:/var/lib/dkms/mok.pub"; do
        cand_key="${pair%%:*}"
        cand_cert="${pair#*:}"
        if [ -f "$cand_key" ] && [ -f "$cand_cert" ]; then
            MOK_KEY="$cand_key"
            MOK_CERT="$cand_cert"
            break
        fi
    done
fi

# Is this certificate trusted by the firmware?  Match on the SHA-1 hash, not
# the subject: subjects are free text and stale keys sharing a CN are common.
mok_enrolled() {
    local fp
    fp=$(openssl x509 -inform DER -in "$1" -noout -fingerprint -sha1 2>/dev/null \
        | sed 's/.*=//' | tr -d ':' | tr 'A-Z' 'a-z')
    [ -n "$fp" ] || return 1
    mokutil --list-enrolled 2>/dev/null | tr -d ':' | tr -d ' \n' \
        | tr 'A-Z' 'a-z' | grep -q "$fp"
}

SIGNING=0
if [ -n "$MOK_KEY" ] && [ -n "$MOK_CERT" ] \
   && [ -f "$MOK_KEY" ] && [ -f "$MOK_CERT" ]; then
    if command -v mokutil >/dev/null 2>&1 && mok_enrolled "$MOK_CERT"; then
        SIGNING=1
    else
        echo "WARN: found a signing key at $MOK_CERT but it is not enrolled in MOK." >&2
        echo "      Modules signed with it will still be rejected under Secure Boot." >&2
        echo "      Enrol it: sudo mokutil --import $MOK_CERT   (set a one-time" >&2
        echo "      password, reboot, complete the MOK manager prompt)." >&2
    fi
fi

if [ "$SB_ENFORCING" = 1 ] && [ "$SIGNING" = 0 ] \
   && [ "${RCRAID_ALLOW_SECURE_BOOT:-0}" != 1 ]; then
    cat >&2 <<'EOF'
ERROR: Secure Boot is enabled and no enrolled signing key was found.

rcraid.ko would be unsigned, so the kernel refuses to load it (insmod:
"Key was rejected by service"; dmesg: "Loading of unsigned module is
rejected").  Installing now would look successful but leave the array
absent after the next reboot.

Either disable Secure Boot in BIOS setup, or create and enrol a Machine
Owner Key first (DKMS signs with it automatically from then on):

  sudo openssl req -new -x509 -newkey rsa:2048 -nodes -days 36500 \
      -subj "/CN=rcraid module signing/" \
      -keyout MOK.priv -outform DER -out MOK.der
  sudo mokutil --import MOK.der
  # set a one-time password, reboot, complete the MOK manager prompt

Keep MOK.priv somewhere permanent — a key whose private half is lost cannot
sign anything, and the enrolled certificate cannot be reused.  Then re-run
this script; it verifies the signature of what DKMS installed.

Override with RCRAID_ALLOW_SECURE_BOOT=1 if you have signed the module by
hand (see INSTALL.md, "Secure Boot").
EOF
    exit 1
fi

# Tell DKMS which key to sign with, so kernel-update rebuilds inherit it.
if [ "$SIGNING" = 1 ]; then
    if grep -q 'framework.conf.d' /usr/sbin/dkms 2>/dev/null; then
        install -d /etc/dkms/framework.conf.d
        cat > /etc/dkms/framework.conf.d/rcraid-signing.conf <<EOF
# Written by rcraid install-dkms.sh.  DKMS sources this when signing, so every
# rebuild — including automatic ones on kernel updates — is signed with the
# key the firmware trusts.
mok_signing_key="$MOK_KEY"
mok_certificate="$MOK_CERT"
EOF
        chmod 0644 /etc/dkms/framework.conf.d/rcraid-signing.conf
        echo "==> DKMS signing configured ($MOK_KEY)"
    else
        echo "WARN: this DKMS does not read /etc/dkms/framework.conf.d; signing" >&2
        echo "      relies on the distribution default key." >&2
    fi
    echo
fi

echo "==> rcraid install-dkms.sh  (driver ${DRIVER_VERSION}, dkms package ${PKG_NAME}-${PKG_VERSION})"
echo

# ----------------------------------------------------------------------------
# 1. Detect array subsystem_vendor.
#
# The AMD chipset reports every downstream NVMe as PCI 1022:b000, including
# the OS drive.  Subsystem vendor used to be the only way to tell members
# apart, but on a single-vendor box (every drive from the same OEM) it is
# identical for all of them and separates nothing — issue #8.  The value is
# still rendered into the modprobe drop-in as a coarse filter, while the
# binding decision that actually matters is made on the RAIDCore metadata in
# rcraid-bind.  Heuristic here: pick the vendor that has >= 2 devices.  If
# there's a tie, list them and let the user pick.
# ----------------------------------------------------------------------------

declare -A vendor_count
declare -A vendor_bdfs
while read -r bdf; do
    [ -n "$bdf" ] || continue
    sv=$(cat "/sys/bus/pci/devices/$bdf/subsystem_vendor")
    vendor_count[$sv]=$((${vendor_count[$sv]:-0} + 1))
    vendor_bdfs[$sv]="${vendor_bdfs[$sv]:-} $bdf"
done < <(lspci -d 1022:b000 -D | awk '{print $1}')

if [ ${#vendor_count[@]} -eq 0 ]; then
    cat >&2 <<EOF
No PCI 1022:b000 devices found.  This box doesn't appear to have any
AMD-RAID Bottom controllers.  Aborting.

If the BIOS RAID mode is enabled and you still see no devices, check
that you're not in AHCI mode and that drives are seated.
EOF
    exit 1
fi

candidates=()
for sv in "${!vendor_count[@]}"; do
    if [ "${vendor_count[$sv]}" -ge 2 ]; then
        candidates+=("$sv")
    fi
done

if [ ${#candidates[@]} -eq 0 ]; then
    echo "Found these 1022:b000 devices but no vendor has >= 2 (so no array detected):" >&2
    for sv in "${!vendor_count[@]}"; do
        echo "  subsystem_vendor $sv → ${vendor_bdfs[$sv]}" >&2
    done
    echo >&2
    echo "rcraid needs a multi-drive array.  Aborting." >&2
    exit 1
elif [ ${#candidates[@]} -eq 1 ]; then
    SUBSYSTEM_VENDOR="${candidates[0]}"
    echo "Detected array members: ${vendor_bdfs[$SUBSYSTEM_VENDOR]# }"
    echo "Subsystem vendor: $SUBSYSTEM_VENDOR"
else
    echo "Multiple candidate arrays detected:"
    i=1
    for sv in "${candidates[@]}"; do
        echo "  [$i] subsystem_vendor $sv → ${vendor_bdfs[$sv]# }"
        i=$((i + 1))
    done
    n=${#candidates[@]}
    while :; do
        read -rp "Pick one [1-$n, default 1]: " choice
        choice="${choice:-1}"
        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "$n" ]; then
            break
        fi
        echo "  invalid — enter a number between 1 and $n"
    done
    SUBSYSTEM_VENDOR="${candidates[$((choice - 1))]}"
fi

# Warn when the filter cannot discriminate: on these boxes safe_subsys_vendor
# matches the drive carrying / just as well as the members, so relying on it
# would be relying on nothing.
ROOT_BDF=""
ROOT_SRC="$(findmnt -no SOURCE / 2>/dev/null || true)"
if [ -n "$ROOT_SRC" ] && [ -b "$ROOT_SRC" ]; then
    while read -r kname ktype; do
        [ "$ktype" = disk ] || continue
        [ -n "$kname" ] || continue
        link=$(readlink -f "/sys/block/$kname/device" 2>/dev/null) || continue
        case "$link" in
            */nvme/*)
                ROOT_BDF="${link%/nvme/*}"
                ROOT_BDF="${ROOT_BDF##*/}"
                break
                ;;
        esac
    done < <(lsblk -nrso KNAME,TYPE "$ROOT_SRC" 2>/dev/null)
fi

if [ -n "$ROOT_BDF" ] && [ -r "/sys/bus/pci/devices/$ROOT_BDF/subsystem_vendor" ] \
   && [ "$(cat "/sys/bus/pci/devices/$ROOT_BDF/subsystem_vendor")" = "$SUBSYSTEM_VENDOR" ]; then
    echo
    echo "NOTE: the drive backing / ($ROOT_BDF) reports the same subsystem_vendor"
    echo "      ($SUBSYSTEM_VENDOR) as the array, so safe_subsys_vendor cannot tell"
    echo "      them apart.  That is normal on a single-vendor box: the udev rule"
    echo "      and rcraid-bind gate on RAIDCore metadata at LBA 0x5000 instead and"
    echo "      leave this drive bound to nvme."
fi
echo

# ----------------------------------------------------------------------------
# 2. Stage sources under /usr/src/<pkg>-<ver> and run dkms install.
# ----------------------------------------------------------------------------

echo "==> staging sources at $DKMS_SRC"
rm -rf "$DKMS_SRC"
mkdir -p "$DKMS_SRC"
cp -r \
    "$SRC_DIR"/Makefile \
    "$SRC_DIR"/dkms.conf \
    "$SRC_DIR"/VERSION \
    "$SRC_DIR"/rc_*.c \
    "$SRC_DIR"/rc_*.h \
    "$DKMS_SRC/"

# Substitute the package version into the staged dkms.conf.
sed -i "s/@PKGVER@/$PKG_VERSION/g" "$DKMS_SRC/dkms.conf"

# Bake the source revision into the staged tree — it has no .git, so the
# Makefile reads .rcraid_rev to stamp the module banner and modinfo.
if rev=$(git -C "$SRC_DIR" describe --always --dirty 2>/dev/null); then
    echo "$rev" > "$DKMS_SRC/.rcraid_rev"
fi

# Upgrade path: retire any OTHER registered rcraid version first, so two
# driver releases never coexist in the dkms tree (the old module would
# shadow or race the new one in updates/dkms).
while read -r oldver; do
    [ -n "$oldver" ] || continue
    [ "$oldver" = "$PKG_VERSION" ] && continue
    echo "==> dkms remove (retiring old version ${PKG_NAME}/${oldver})"
    dkms remove -m "$PKG_NAME" -v "$oldver" --all 2>&1 | sed 's/^/    /'
    rm -rf "/usr/src/${PKG_NAME}-${oldver}"
done < <(dkms status "$PKG_NAME" 2>/dev/null \
             | sed -n "s#^${PKG_NAME}[/,][ ]*\([^,/:]*\)[,/:].*#\1#p" | sort -u)

# A re-run of this script for the SAME version must not assume a clean
# slate: skip `add` if the package is already registered, and force
# build/install so dkms rebuilds from the freshly staged sources instead
# of silently keeping the previously installed module.
if [ -n "$(dkms status -m "$PKG_NAME" -v "$PKG_VERSION" 2>/dev/null)" ]; then
    echo "==> dkms add (skipped — ${PKG_NAME}/${PKG_VERSION} already registered)"
else
    echo "==> dkms add"
    dkms add -m "$PKG_NAME" -v "$PKG_VERSION" 2>&1 | sed 's/^/    /'
fi

echo "==> dkms build"
dkms build -m "$PKG_NAME" -v "$PKG_VERSION" --force 2>&1 | sed 's/^/    /'

echo "==> dkms install"
dkms install -m "$PKG_NAME" -v "$PKG_VERSION" --force 2>&1 | sed 's/^/    /'
echo

# ----------------------------------------------------------------------------
# 2b. Prove what DKMS installed is actually signed.
#
# DKMS signs during build, but a missing/unreadable key makes it print a
# warning and carry on with an unsigned module.  Under Secure Boot that is
# only visible at the next boot, as a missing array — so check here instead.
# ----------------------------------------------------------------------------

if [ "$SIGNING" = 1 ] || [ "$SB_ENFORCING" = 1 ]; then
    echo "==> verifying module signatures"
    UNSIGNED=0
    for ko in /lib/modules/*/updates/dkms/${PKG_NAME}.ko*; do
        [ -e "$ko" ] || continue
        # <ko> is .../<kver>/updates/dkms/<name>.ko[.zst] — climb three levels.
        KVER="$(basename "$(dirname "$(dirname "$(dirname "$ko")")")")"
        SIGNER="$(modinfo -F signer "$ko" 2>/dev/null || true)"
        if [ -n "$SIGNER" ]; then
            echo "    $KVER: signed by $SIGNER"
        else
            echo "    $KVER: UNSIGNED ($ko)" >&2
            UNSIGNED=1
        fi
    done

    if [ "$UNSIGNED" = 1 ]; then
        # RCRAID_ALLOW_SECURE_BOOT=1 means "I sign these myself": warn rather
        # than undo the install the user explicitly asked for.
        if [ "$SB_ENFORCING" = 1 ] && [ "${RCRAID_ALLOW_SECURE_BOOT:-0}" != 1 ]; then
            echo >&2
            echo "ERROR: at least one installed ${PKG_NAME} module is unsigned while" >&2
            echo "       Secure Boot is enforcing — the kernel will reject it and the" >&2
            echo "       array will be missing at the next boot." >&2
            echo "       Re-sign by hand (INSTALL.md, \"Secure Boot\") then re-run" >&2
            echo "       scripts/verify-boot-safety.sh.  Do not reboot yet." >&2
            exit 1
        fi
        echo "WARN: unsigned module(s) installed." >&2
        if [ "$SB_ENFORCING" = 1 ]; then
            echo "      Secure Boot is enforcing and RCRAID_ALLOW_SECURE_BOOT=1 was" >&2
            echo "      given, so signing is left to you: sign each module listed" >&2
            echo "      above before rebooting (INSTALL.md, \"Secure Boot\"), then" >&2
            echo "      run scripts/verify-boot-safety.sh." >&2
        else
            echo "      Harmless while Secure Boot is off, fatal the moment it is" >&2
            echo "      switched back on." >&2
        fi
    fi
    echo
fi

# ----------------------------------------------------------------------------
# 3. Lay down the udev rule, helper, and modprobe drop-in.
# ----------------------------------------------------------------------------

echo "==> installing /usr/sbin/rcraid-bind"
install -m 0755 "$SRC_DIR/packaging/sbin/rcraid-bind" /usr/sbin/rcraid-bind

echo "==> rendering /etc/udev/rules.d/50-rcraid.rules"
sed "s/@SUBSYSTEM_VENDOR@/$SUBSYSTEM_VENDOR/g" \
    "$SRC_DIR/packaging/udev/50-rcraid.rules.in" \
    > /etc/udev/rules.d/50-rcraid.rules
chmod 0644 /etc/udev/rules.d/50-rcraid.rules

echo "==> rendering /etc/modprobe.d/rcraid.conf"
sed "s/@SUBSYSTEM_VENDOR@/$SUBSYSTEM_VENDOR/g" \
    "$SRC_DIR/packaging/modprobe.d/rcraid.conf.in" \
    > /etc/modprobe.d/rcraid.conf
chmod 0644 /etc/modprobe.d/rcraid.conf

echo "==> reloading udev rules"
udevadm control --reload-rules
echo

# On apt-based systems, keep kernel packages out of unattended-upgrades so
# a kernel update (and the DKMS rebuild + initramfs regeneration it needs)
# is always deliberate — an unattended one that broke the rcraid build
# would leave the new kernel unbootable with nobody watching.
if [ -d /etc/apt/apt.conf.d ]; then
    echo "==> installing /etc/apt/apt.conf.d/52-rcraid-kernel-hold"
    install -m 0644 "$SRC_DIR/packaging/apt.conf.d/52-rcraid-kernel-hold" \
        /etc/apt/apt.conf.d/52-rcraid-kernel-hold
    echo "    (kernel packages excluded from unattended-upgrades; after a"
    echo "     deliberate kernel update run scripts/verify-boot-safety.sh)"
    echo
fi

# ----------------------------------------------------------------------------
# 4. Install initramfs hook so the array can serve /, and regenerate the
#    initramfs for the running kernel.
#
# We support both dracut (Fedora/RHEL/Arch/openSUSE) and initramfs-tools
# (Debian/Ubuntu).  If neither is present, the install still succeeds but
# boot-from-RAID won't work — you can only mount the array post-boot.
# ----------------------------------------------------------------------------

INITRAMFS_GENERATOR=""

if command -v dracut >/dev/null 2>&1; then
    INITRAMFS_GENERATOR="dracut"
    echo "==> installing dracut module 95rcraid"
    install -d /usr/lib/dracut/modules.d/95rcraid
    install -m 0755 \
        "$SRC_DIR/packaging/dracut/95rcraid/module-setup.sh" \
        /usr/lib/dracut/modules.d/95rcraid/module-setup.sh

    echo "==> regenerating initramfs (dracut -f)"
    if ! dracut -f 2>&1 | sed 's/^/    /'; then
        echo "ERROR: dracut FAILED — the initramfs does NOT contain rcraid." >&2
        echo "       Do NOT reboot until 'dracut -f' succeeds (a full /boot" >&2
        echo "       is the usual cause), then re-run this script or verify" >&2
        echo "       with scripts/verify-boot-safety.sh." >&2
        exit 1
    fi
elif command -v update-initramfs >/dev/null 2>&1; then
    INITRAMFS_GENERATOR="initramfs-tools"
    echo "==> installing initramfs-tools hook"
    install -d /etc/initramfs-tools/hooks
    install -m 0755 \
        "$SRC_DIR/packaging/initramfs-tools/hooks/rcraid" \
        /etc/initramfs-tools/hooks/rcraid

    echo "==> regenerating initramfs (update-initramfs -u)"
    if ! update-initramfs -u 2>&1 | sed 's/^/    /'; then
        echo "ERROR: update-initramfs FAILED — the initramfs does NOT contain" >&2
        echo "       rcraid.  Do NOT reboot until 'update-initramfs -u'" >&2
        echo "       succeeds (a full /boot is the usual cause), then re-run" >&2
        echo "       this script or verify with scripts/verify-boot-safety.sh." >&2
        exit 1
    fi
else
    echo "==> no initramfs generator found (dracut / update-initramfs)"
    echo "    Skipping initramfs hook install.  The array will only be"
    echo "    available AFTER the root filesystem mounts.  This is fine if"
    echo "    your rootfs is NOT on rcraid; if it is, install dracut or"
    echo "    initramfs-tools and re-run this script."
fi
echo

# ----------------------------------------------------------------------------
# 5. Done.  Tell the user what happens next.
# ----------------------------------------------------------------------------

if grep -q '^rcraid ' /proc/modules; then
    cat <<EOF
NOTE: rcraid is currently loaded, so the running module is still the old
build.  The freshly installed module (and the regenerated initramfs) take
effect on the next reboot.

EOF
fi

cat <<EOF
==> Install complete.

What's installed:
  DKMS module    /usr/src/${PKG_NAME}-${PKG_VERSION}/
                 /lib/modules/\$(uname -r)/updates/dkms/rcraid.ko
  Signing        $([ "$SIGNING" = 1 ] && echo "DKMS signs with ${MOK_KEY} on every rebuild" || echo 'none — module is unsigned (Secure Boot must stay off)')
  Bind helper    /usr/sbin/rcraid-bind
  udev rule      /etc/udev/rules.d/50-rcraid.rules
                 (binds only devices carrying RAIDCore metadata)
  modprobe conf  /etc/modprobe.d/rcraid.conf
                 (enable_writes=1, safe_subsys_vendor=$SUBSYSTEM_VENDOR)
  initramfs      ${INITRAMFS_GENERATOR:-<none — boot-from-RAID disabled>}
$([ -e /etc/apt/apt.conf.d/52-rcraid-kernel-hold ] && echo '  apt hold       /etc/apt/apt.conf.d/52-rcraid-kernel-hold
                 (kernel packages held back from unattended-upgrades)')

After any kernel update, run scripts/verify-boot-safety.sh BEFORE
rebooting — it confirms rcraid is built and present in every installed
kernel's initramfs.

Next: reboot to test the auto-bind path end-to-end.  After reboot:

  lsblk /dev/rcraid0     # should show your volume + partitions
  lspci -d 1022:b000 -k  # array members should show "rcbottom"

To take effect WITHOUT rebooting (one-shot test):

  for bdf in ${vendor_bdfs[$SUBSYSTEM_VENDOR]# }; do
      /usr/sbin/rcraid-bind \$bdf
  done

To remove: sudo ./uninstall-dkms.sh
EOF
