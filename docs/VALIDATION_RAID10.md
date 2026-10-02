# RAID 10 Hardware Validation Runbook

This is the checklist that turns the RAID 10 row from **"Feature-complete —
validation pending"** into **"Baseline — stable."** Nothing here runs on a
development laptop: it is executed on the **array box** (the machine with the
`1022:B000` controller and a firmware-created RAID 10 array). The QEMU rig and
CI exercise the same paths on GitHub's runners, not locally.

Work top to bottom. Steps 1 and 2 are **destructive** — run them against a
scratch array or a scratch partition (`/dev/rcraid0pN`), never against a disk
holding anything you care about.

## Preconditions

- [ ] 4 (or any even number of) NVMe members in a **firmware-created RAID 10**.
- [ ] OS running from a **non-array** drive (or a live USB) for the
      destructive phases.
- [ ] `make KERNELDIR=/lib/modules/$(uname -r)/build` builds clean, zero
      warnings.
- [ ] `fio` installed (`apt-get install fio` / `dnf install fio`).

## Operator surface

| What | How | Meaning |
|---|---|---|
| State | `cat /sys/block/rcraid0/rcraid/state` | `unassembled` / `failed` / `resyncing` / `degraded` / `optimal` |
| Members | `cat /sys/block/rcraid0/rcraid/members` | `N <bdf> live\|failed\|needs-resync\|resyncing`, one per slot |
| Rebuild | `cat /sys/block/rcraid0/rcraid/resync_progress` | `none`, or `memberN cur/tot pct%` |
| Fail a copy | `echo <slot> \| sudo tee /sys/block/rcraid0/rcraid/fail_member` | latch member `slot` failed (0-based) |
| Re-admit | `echo 1 \| sudo tee /sys/bus/pci/devices/<bdf>/rcraid/reset` | reset controller; member returns `needs-resync` |
| Detail | `/sys/kernel/debug/rcraid/volume` | geometry, per-member state, cursor |

`allow_degraded` is a load-time module parameter (`0444`): set it via
`modprobe rcraid allow_degraded=1` or a `modprobe.d` drop-in — it cannot be
changed at runtime.

## 0. Trust gate

- [ ] `dmesg | grep -i UNTRUSTED` is **empty**. If not, the parser vetoed the
      geometry — stop, capture the dmesg line, and treat it as a bug.

## 1. Geometry and the pair convention — do this FIRST

The one assumption never checked against real drives is **which two members
form a mirror column**: the driver assumes adjacent (`{2c, 2c+1}`), the
alternative is strided (`{c, c+cols}`). A wrong pairing does not crash — it
mirrors to the wrong partner and reads across non-partners, i.e. silent
corruption. Resolve it before trusting any write.

- [ ] Confirm geometry: `members` count is even, `cols = members/2`, and the
      debugfs `volume` line agrees with the firmware (`FirstCount × SecondCount`).
- [ ] Falsify the pairing, either:
  - **(a) pre-existing data** — if the array already holds firmware-written
    filesystem data, read a checksummed range through `/dev/rcraid0`. A wrong
    pair map returns interleaved garbage, so a clean read is strong evidence; or
  - **(b) raw compare** — from a state where the members are **not** bound to
    `rcbottom` (live USB before load, or Windows), read a small region past each
    member's `UserDataOffset` and record which two match byte-for-byte.
- [ ] Record the observed pairing here: `PAIRING = ______________`.
      Adjacent expected → proceed. Strided → **STOP**; only the
      copy→member mapping in `rc_volume_phys_for_member` changes (`{c, c+cols}`),
      fix, rebuild, re-run from step 0.

## 2. Data path

- [ ] `sudo ./test_write_path.sh --yes /dev/rcraid0` (destructive) passes all
      four phases: fragmented buffered writes + `fsync`, stripe-unaligned
      direct writes, `mkfs.ext4` + `e2fsck -fn`, and a clean dmesg tripwire.
- [ ] `fsync`, FUA and `fstrim` behave (no "rejected" / "failed SC/SCT" lines).
- [ ] Reads round-robin across each pair; writes/discards reach **both** copies.

## 3. Degraded failover (per column)

- [ ] Fail one copy of one pair:
      `echo 0 | sudo tee /sys/block/rcraid0/rcraid/fail_member`
- [ ] `state` → `degraded`; I/O keeps flowing from the survivor; `members`
      shows slot 0 `failed`.
- [ ] Fatal boundary: fail **both** copies of a pair. `state` → `failed` and
      new I/O fast-fails (the volume is unserviceable, not silently wrong).
- [ ] Re-admit a survivor so the array is serviceable again for step 4.

## 4. Rebuild / resync

- [ ] Reset a failed copy:
      `echo 1 | sudo tee /sys/bus/pci/devices/<bdf>/rcraid/reset`
- [ ] It returns `needs-resync`; the engine rebuilds it **from its pair
      partner** (the only member holding the same column's stripes).
- [ ] Watch `resync_progress` advance while running **concurrent writes** to
      `/dev/rcraid0`.
- [ ] When it reaches `optimal`, prove the sourcing: fail the *partner* and read
      back data written during the rebuild — it must be present and correct.

## 5. Boot-degraded assembly

- [ ] With `allow_degraded=1`, make one copy of one pair absent, reboot: the
      array assembles `degraded` and rebuilds the missing copy when it returns.
- [ ] Make **both** copies of a pair absent, reboot: assembly **refuses** (no
      `/dev/rcraid0`) rather than exposing an unreadable volume.

## 6. Install and reboot

- [ ] Install Kubuntu onto `/dev/rcraid0`, reboot.
- [ ] `scripts/verify-boot-safety.sh` (module + initramfs present for every
      kernel), then `bench.sh` and `test_driver.sh`.

## 7. Promote — only after every box above is checked

- [ ] `README.md`: RAID 10 row → `✅ **Baseline — stable**`; drop the
      "pending real-hardware validation" wording from the warning and bullet.
- [ ] `docs/STATUS.md`: dated `## <date> — RAID10 hardware validation` entry;
      move RAID10 out of "Not yet".
- [ ] CI: add `raid10`, `raid10-degraded`, `raid10-degraded-boot` to the
      `qemu-rig` matrix.

## Results

| # | Check | Result | Notes |
|---|---|---|---|
| 0 | Trust gate | | |
| 1 | Pair convention | | adjacent / strided |
| 2 | Write path | | |
| 3 | Degraded + fatal boundary | | |
| 4 | Rebuild + concurrent writes | | |
| 5 | Boot-degraded + whole-pair refusal | | |
| 6 | Install / reboot / boot-safety | | |
