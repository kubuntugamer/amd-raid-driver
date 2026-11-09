# shellcheck shell=bash
# Shared Secure Boot detection for the rcraid installers.  Sourced (not
# executed) by install-dkms.sh and install-livecd.sh, which need the same
# answer to the same question: will this kernel refuse our unsigned module?
#
# rcraid.ko is unsigned, so a Secure-Boot-enforcing kernel rejects it with
# insmod's "Key was rejected by service" (dmesg: "Loading of unsigned module
# is rejected").  The installers call secure_boot_enforcing before doing any
# real work and abort with instructions instead.
#
# The SecureBoot EFI variable is read directly rather than via mokutil: the
# live-CD preflight runs before any packages are installed.  efivarfs layout
# is 4 bytes of attributes followed by the data; for SecureBoot the data is
# one byte, 1 = enforcing.  No efivarfs entry at all means a BIOS/CSM boot,
# where Secure Boot doesn't exist.
#
# The 5 bytes must be read from offset 0: efivarfs does not support seeking,
# so `od -j4` (and `dd skip=4`) fail with "Illegal seek" / "cannot skip past
# end of combined input" on every real efivar, which would make every probe
# take the fail-closed path below and report Secure Boot as enforcing even on
# machines that have it switched off.
#
# A variable that exists but can't be read (firmware EIO quirks happen on
# real boards) — or reads back empty/truncated, which od reports as SUCCESS
# with no output — FAILS CLOSED: assume enforcing, warn, and let the user
# decide via RCRAID_ALLOW_SECURE_BOOT=1.  Silently treating an anomalous
# variable as "not enforcing" would reproduce the exact only-fails-at-next-
# boot behavior this check exists to prevent.
#
# RCRAID_EFIVARS_DIR overrides the efivarfs path (for tests only).

secure_boot_enforcing() {
    local var byte raw
    for var in "${RCRAID_EFIVARS_DIR:-/sys/firmware/efi/efivars}"/SecureBoot-*; do
        [ -e "$var" ] || break
        # Read attributes + data in one go and take the 5th field; see the
        # seek note above.  The read is kept out of a pipeline so its exit
        # status survives: `od | awk` would report awk's status and turn a
        # failed read into an empty value instead of the unreadable warning.
        if ! raw=$(od -An -tu1 -N5 "$var" 2>/dev/null); then
            echo "WARN: SecureBoot EFI variable exists but can't be read — assuming" >&2
            echo "      Secure Boot is ENFORCING (override: RCRAID_ALLOW_SECURE_BOOT=1)" >&2
            return 0
        fi
        byte=$(printf '%s\n' "$raw" | awk '{print $5}')
        byte="${byte//[[:space:]]/}"
        if [ -z "$byte" ]; then
            echo "WARN: SecureBoot EFI variable exists but is empty/truncated — assuming" >&2
            echo "      Secure Boot is ENFORCING (override: RCRAID_ALLOW_SECURE_BOOT=1)" >&2
            return 0
        fi
        [ "$byte" = "1" ]
        return
    done
    return 1
}
