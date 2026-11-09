# RAID 5 Hardware Validation Runbook

This is the checklist for the **read-only** RAID 5 path. Unlike
[VALIDATION_RAID10.md](VALIDATION_RAID10.md), there is no destructive write
phase to run: the driver has no parity write path, no rebuild, and refuses
every write and discard. The remaining open question is the **parity
placement** (issue #6), not whether the volume can be written.

## What is already confirmed on hardware

A RAIDXpert2 BIOS-created **3-disk RAID 5** (3× Sabrent Rocket Q 3.7 TB,
`1022:B000`) produced this committed `RC_LogicalDevice` record:

| Field | Observed value |
|---|---|
| `DeviceType` | `0x1BF5` |
| `Devices` | `3` |
| `FirstCount` / `SecondCount` | `1` / `3` |
| `Capacity` | `15,625,838,592` sectors |
| per-member `UserDataSize` | `7,812,919,296` sectors |
| per-member `UserDataOffset` | `0x105000` sectors (1,069,547,520 B) |
| `ChunkSize` / `ChunkIndex` | `0` / `3` → **256 KiB** stripe |

Capacity is exactly `(N-1) × UserDataSize`, which is RAID 5. This also
settles the DeviceType encoding: `0x1BF5` is **RAID 5**, `0x1BFA` is
**RAID 6**. The vendor dispatch agrees — it derives the parity count as
`(DeviceType == 0x1BFA) + 1`. The driver's constants were swapped before
2026-11-08 and are now corrected.

What is still **not** confirmed is the layout of the parity itself
(left-symmetric, row-rotating, dense packing, 256 KiB chunks) against data
written by the vendor firmware/driver. A freshly created array is zeroed, so
XOR checks on it are vacuous — all-zero data and all-zero parity agree with
every candidate layout.

## Fast path — one command

`scripts/validate-raid5.sh` runs the non-destructive checks on the array box:

```sh
sudo ./scripts/validate-raid5.sh              # sampled reads, read-only checks
sudo ./scripts/validate-raid5.sh --full-read  # also read the whole volume
```

It never writes to the array. It does verify that a write is **refused**.
Paste the whole output for sign-off.

## Preconditions

- [ ] At least 3 NVMe members in a **firmware-created RAID 5**.
- [ ] No foreign RAID metadata on the members (a drive from HighPoint/Intel/
      mdadm must be initialized before the AMD array is created).
- [ ] `rcraid` loaded against the three array members. On a box that also has
      a spare/unconfigured `1022:B000` disk, bind **only** the members — a
      disk whose valid RAIDCore metadata does not name it is refused by the
      legacy-fallback guard, but the installer's subsystem-vendor filter will
      still attempt to probe it.
- [ ] Secure Boot: the out-of-tree module must be signed and trusted (MOK) or
      Secure Boot disabled. An unsigned `insmod` fails with
      `Key was rejected by service`.
- [ ] `make KERNELDIR=/lib/modules/$(uname -r)/build` builds clean.

## Operator surface

| What | How | Meaning |
|---|---|---|
| State | `cat /sys/block/rcraid0/rcraid/state` | RAID5 is `optimal` or `failed`; there is **no** `degraded`/`resyncing` |
| Members | `cat /sys/block/rcraid0/rcraid/members` | one line per LD position |
| Detail | `/sys/kernel/debug/rcraid/volume` | level, member count, per-member state |
| Read-only | `blockdev --getro /dev/rcraid0` | must be `1` |
| Load record | `dmesg \| grep rc_volume_parse_logical_device` | raw DeviceType/counts/capacity/chunk |

`enable_writes=1` cannot make a RAID 5 volume writable: the driver records a
geometry veto for every parity array, and the write path returns an error.

**Do not use `fail_member` on a RAID 5 array.** With one member gone, the
volume goes straight to `failed`; there is no reconstruction/rebuild path.

## 0. Trust gate

- [ ] `dmesg | grep -i UNTRUSTED` contains **only** the expected
      `RAID 5/6 is assembled READ-ONLY …` reason. Any capacity-mismatch or
      unknown-chunk line is a bug — capture it.

## 1. Level and geometry

- [ ] `debugfs volume` reports `level: raid5` and `members: N`.
- [ ] The parse line shows `devtype=0x1bf5`, `devices=N`, and
      `capacity = UserDataSize × (N-1)`.
- [ ] `blockdev --getsize64 /dev/rcraid0 / 512` equals that capacity.
- [ ] The chunk resolves to a known stripe (index 1/2/3 → 64/128/256 KiB).

## 2. Read path

- [ ] `scripts/validate-raid5.sh` sampled reads pass at 0/25/50/75/100 % and
      are stable across a cache drop.
- [ ] A write to `/dev/rcraid0` fails and the disk stays read-only.
- [ ] No new `rcraid` I/O error / timeout / reset lines in dmesg.

## 3. Layout confirmation — the open item (issue #6)

Do **one** of these before calling the parity map confirmed.

### 3a. Vendor-written data (non-destructive, preferred)

- [ ] While booted with the AMD RAIDXpert2/Windows driver (or using an array
      that already holds data), write a known pattern — e.g. a file of random
      data, or `mkfs` + files — and record its checksum.
- [ ] Boot Linux with `rcraid`, read the same range through `/dev/rcraid0`
      (or mount read-only), and compare. A clean match across several
      stripe-sized ranges is strong evidence; interleaved garbage means the
      mapping is wrong.
- [ ] Record the observed layout here: `LAYOUT = ______________`.

### 3b. Direct-member stamp (destructive; validates the driver against the documented model, not the firmware)

This proves the driver reads the *documented* map; it cannot prove the vendor
writes that map. It is still useful to separate a read-path bug from a
layout mismatch.

The documented map (`src/patch_parity_math.c`) for N=3, 256 KiB chunks:

| row | parity member | data members (stripe 0, stripe 1) | physical offset on every member |
|---|---|---|---|
| 0 | 2 | 0, 1 | `UserDataOffset + 0 × chunk` |
| 1 | 1 | 0, 2 | `UserDataOffset + 1 × chunk` |
| 2 | 0 | 1, 2 | `UserDataOffset + 2 × chunk` |

Parity rotates backward: `parity_member = (N-1) - (row % N)`; data chunks are
the non-parity members in ascending order; the row's chunks are packed at the
same row offset on each member. Stamp `data0`, `data1`, `P = data0 XOR data1`
accordingly, load the driver, and verify `/dev/rcraid0` returns the logical
data. Restore zeros afterwards (the array started zeroed).

- [ ] Stamp + read-back matches for at least several full rows.
- [ ] Restore the stamped region.

## 4. Explicitly not testable yet

- Parity write / read-modify-write, and discard.
- Member rebuild and degraded reads (RAID5 has no single-member reconstruction
  path in this driver).
- Boot-from-RAID5.
- OCE / ORLM / metadata writer.

## Results

| # | Check | Result | Notes |
|---|---|---|---|
| 0 | Trust gate | | |
| 1 | Level / geometry / capacity | | 0x1BF5, N=3, `(N-1)` × user size |
| 2 | Read path + read-only enforcement | | |
| 3 | Parity placement | | issue #6 |
| 4 | Boot / install | | blocked on writes |

## Promote

Do **not** mark RAID 5 stable until the parity write path, placement
confirmation, and rebuild exist. When step 3 signs off, the README row may
move from "level/geometry unconfirmed" to "assembles and reads on hardware;
placement confirmed", still read-only.
