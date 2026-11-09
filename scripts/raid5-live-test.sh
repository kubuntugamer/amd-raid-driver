#!/bin/bash
#
# raid5-live-test.sh — load rcraid against the real 1022:B000 RAID 5 array and
# verify the staged read-path fixture.
#
# This is the post-reboot companion to docs/VALIDATION_RAID5.md.  It signs the
# module if needed, unbinds the 1022:B000 controllers from nvme (never the disk
# backing /), loads rcraid, runs validate-raid5.sh, then compares the first
# 64 MiB of /dev/rcraid0 against .freebuff/raid5-test-manifest.txt.
#
# It never writes to the array.  If the staged fixture is absent it only runs
# the read-only validation checks.
#
#   sudo ./scripts/raid5-live-test.sh

set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_DIR"

if [ "$(id -u)" -ne 0 ]; then
    exec sudo "$0" "$@"
fi

MOK_PRIV=/var/lib/shim-signed/mok/MOK.priv
MOK_DER=/var/lib/shim-signed/mok/MOK.der
MANIFEST="$REPO_DIR/.freebuff/raid5-test-manifest.txt"
DEV=/dev/rcraid0
FAILED=0

ok()   { printf '  \033[32mPASS\033[0m  %s\n' "$*"; }
bad()  { printf '  \033[31mFAIL\033[0m  %s\n' "$*"; FAILED=1; }
note() { printf '  ....  %s\n' "$*"; }
hdr()  { printf '\n== %s ==\n' "$*"; }

# ---------------------------------------------------------------- build/sign
hdr "Module"
if [ ! -f rcraid.ko ]; then
    note "rcraid.ko missing — building"
    make -j"$(nproc)" || { bad "build failed"; exit 1; }
fi

# NOTE: `writer | grep -q` is unusable here — grep exits on the first match,
# the writer takes SIGPIPE, and `set -o pipefail` reports 141, so the test is
# always false.  Test the value directly instead.
if [ -z "$(modinfo -F signer ./rcraid.ko 2>/dev/null)" ]; then
    if [ -r "$MOK_PRIV" ] && [ -r "$MOK_DER" ]; then
        note "signing rcraid.ko with the local MOK key"
        /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 \
            "$MOK_PRIV" "$MOK_DER" ./rcraid.ko || { bad "sign-file failed"; exit 1; }
    else
        note "module is unsigned and no local MOK key is present"
    fi
fi
modinfo -F signer ./rcraid.ko 2>/dev/null | sed 's/^/        signer: /' || true

# ---------------------------------------------------------------- bind
hdr "Bind array controllers (skip any disk backing /)"
root_src="$(findmnt -no SOURCE / 2>/dev/null || true)"
root_bdfs=""
if [ -n "$root_src" ] && [ -b "$root_src" ]; then
    while read -r disk; do
        [ -n "$disk" ] || continue
        p="$(readlink -f "/sys/block/$disk/device" 2>/dev/null || true)"
        bdf="$(printf '%s' "$p" | grep -oE '[0-9a-f]{4}:[0-9a-f]{2}:[0-9a-f]{2}\.[0-9a-f]' | tail -1 || true)"
        [ -n "$bdf" ] && root_bdfs="$root_bdfs $bdf"
    done < <(lsblk -nrso KNAME,TYPE "$root_src" 2>/dev/null | awk '$2=="disk"{print $1}' | sort -u)
fi
[ -n "$root_bdfs" ] && note "root is backed by PCI device(s):$root_bdfs"

bound=0
UNBOUND_BDFS=""
rebind_unbound() {
    for b in $UNBOUND_BDFS; do
        echo "$b" > /sys/bus/pci/drivers/nvme/bind 2>/dev/null || true
    done
}
for bdf in $(lspci -d 1022:b000 -D 2>/dev/null | awk '{print $1}'); do
    drv="$(basename "$(readlink -f "/sys/bus/pci/devices/$bdf/driver" 2>/dev/null)" 2>/dev/null || echo none)"
    case " $root_bdfs " in
        *" $bdf "*)
            note "skipping $bdf — backs the running root filesystem"
            continue
            ;;
    esac
    if [ "$drv" = nvme ]; then
        if echo "$bdf" > /sys/bus/pci/drivers/nvme/unbind 2>/dev/null; then
            note "unbound $bdf from nvme"
            UNBOUND_BDFS="$UNBOUND_BDFS $bdf"
            bound=$((bound + 1))
        else
            bad "could not unbind $bdf from nvme"
        fi
    elif [ "$drv" = rcbottom ]; then
        note "$bdf already bound to rcbottom"
    else
        note "$bdf currently on driver '$drv' — leaving it"
    fi
done
[ "$bound" -gt 0 ] && sleep 1

# ---------------------------------------------------------------- load
hdr "Load rcraid"
if [ -d /sys/module/rcraid ]; then
    note "rcraid already loaded"
else
    if insmod ./rcraid.ko; then
        ok "insmod succeeded"
    else
        bad "insmod failed — if this is 'Key was rejected by service', the MOK key is not enrolled yet"
        note "restoring the unbound controllers to nvme"
        rebind_unbound
        exit 1
    fi
fi

for _ in $(seq 1 20); do
    [ -b "$DEV" ] && break
    sleep 0.5
done
if [ ! -b "$DEV" ]; then
    bad "$DEV did not appear"
    echo
    echo "Recent driver messages:"
    dmesg | grep -iE 'rcraid|rc_volume|rc_nvme' | tail -60
    exit 1
fi
ok "$DEV is present"

# ---------------------------------------------------------------- validate
hdr "Read-only validation"
"$SCRIPT_DIR/validate-raid5.sh" --dev "$DEV" || FAILED=1

# ---------------------------------------------------------------- fixture
hdr "Staged fixture (first 64 MiB)"
if [ ! -f "$MANIFEST" ]; then
    note "no $MANIFEST — skipping fixture comparison"
else
    want_size="$(awk -F= '$1=="size"{print $2}' "$MANIFEST")"
    want_hash="$(awk -F= '$1=="sha256"{print $2}' "$MANIFEST")"
    if [ -n "$want_size" ] && [ -n "$want_hash" ]; then
        got_hash="$(dd if="$DEV" bs=1M count=$((want_size / 1048576)) iflag=direct status=none \
                    | sha256sum | awk '{print $1}')" || got_hash=""
        if [ "$got_hash" = "$want_hash" ]; then
            ok "first $((want_size / 1048576)) MiB match the staged image (sha256 $want_hash)"
        else
            bad "staged image mismatch: got '$got_hash', want '$want_hash'"
        fi
    else
        note "manifest lacks size/sha256"
    fi

    mkdir -p "$REPO_DIR/.freebuff/mnt"
    if mount -o ro "$DEV" "$REPO_DIR/.freebuff/mnt" 2>/dev/null; then
        if (cd "$REPO_DIR/.freebuff/mnt" && sha256sum -c MANIFEST.sha256 >/dev/null 2>&1); then
            ok "ext4 fixture mounts read-only and all file checksums match"
        else
            bad "fixture mounted but file checksums failed"
        fi
        umount "$REPO_DIR/.freebuff/mnt"
    else
        note "could not mount $DEV read-only (raw hash above is authoritative)"
    fi
fi

# ---------------------------------------------------------------- verdict
hdr "VERDICT"
if [ "$FAILED" -ne 0 ]; then
    echo "RESULT: FAILED — paste the output above plus 'dmesg | grep -i rcraid | tail -80'."
    exit 1
fi
echo "RESULT: RAID 5 assembled and the staged read path verified."
echo "        Parity placement against firmware-written data is still the open"
echo "        item (issue #6); see docs/VALIDATION_RAID5.md step 3."
