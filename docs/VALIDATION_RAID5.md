# RAID 5 Hardware Validation Runbook

This is the checklist for the **read-only** RAID 5 path. Unlike
[VALIDATION_RAID10.md](VALIDATION_RAID10.md), there is no destructive write
phase to run: the driver has no parity write path, no rebuild, and refuses
every write and discard. The parity **placement** is no longer the open
question — it is settled against the vendor core (§3) and the driver's map
already follows it. What remains is corroboration on vendor-written bytes and,
above all, the write path itself.

## What is already confirmed on hardware

A RAIDXpert2 BIOS-created **3-disk RAID 5** (3× Sabrent Rocket Q 3.7 TB,
`1022:B000`) produced this committed `RC_LogicalDevice` record. That array
was removed on 2026-11-09, so the record below is the surviving evidence and
the runbook has no current fixture to run against — re-running it requires
creating a new array in the RAIDXpert2 UEFI utility first:

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

The layout itself is now read out of the vendor core — dedicated parity on
the last member, dense packing, 256 KiB chunks (`docs/REVERSE_ENGINEERING.md`
item 7) — and the RAID 6 Q encoding is settled as EVENODD with prime 17
(item 8). What is still **not** confirmed is that layout against data written
by the vendor firmware/driver. A freshly created array is zeroed, so
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

- [x] `dmesg | grep -i UNTRUSTED` contains **only** the expected
      `RAID 5/6 is assembled READ-ONLY …` reason. Any capacity-mismatch or
      unknown-chunk line is a bug — capture it. (2026-11-08: only the
      read-only reason present.)

## 1. Level and geometry

- [x] `debugfs volume` reports `level: raid5` and `members: N`.
- [x] The parse line shows `devtype=0x1bf5`, `devices=N`, and
      `capacity = UserDataSize × (N-1)`.
- [x] `blockdev --getsize64 /dev/rcraid0 / 512` equals that capacity.
- [x] The chunk resolves to a known stripe (index 1/2/3 → 64/128/256 KiB).

All four above were checked on the 3-disk array, 2026-11-08: `level=raid5`,
`members=3`, `devtype=0x1bf5`, `devices=3`, capacity `15,625,838,592`
sectors, `chunk_idx=3` → 256 KiB.

## 2. Read path

- [x] `scripts/validate-raid5.sh` sampled reads pass at 0/25/50/75/100 % and
      are stable across a cache drop.
- [x] A write to `/dev/rcraid0` fails and the disk stays read-only.
- [x] No new `rcraid` I/O error / timeout / reset lines in dmesg.

All three were checked on 2026-11-08: 18 checks PASS. Note that the script's
read-only-veto check could not pass before `6ced208` — `dmesg | grep -q` under
`set -o pipefail` returns 141 from SIGPIPE, so it always reported "veto line
not found". See the procedural note at the end of §3b.

## 3. Layout confirmation — answered from the vendor binary (issue #6)

**The vendor placement is dedicated parity on the last member, not the
rotating map this tree used to implement.** Read out of the DWARF-bearing
`rcblob.x86_64` (SDK 9.3.0-00283); the derivation is in
`docs/REVERSE_ENGINEERING.md` ("RAID 5/6 layout"):

| | vendor blob | this driver (since 2026-11-08) |
|---|---|---|
| RAID 5 data member | `(lba >> chunk_pow) % (N-1)` → members `0..N-2` | same |
| RAID 5 parity member | **`N-1`, every row** | `N-1`, every row |
| RAID 6 | data `0..N-3`, P/Q on `N-2`/`N-1` | same (P/Q order assumed) |
| row stride | `(N - parity_count) × chunk`, packed per row | same |

`patch_parity_math.c` used to rotate (`parity_pos = (N-1) - (row % N)`), which
disagreed with the vendor on two rows in three for `N=3`; it now implements the
dedicated layout. The change showed up on the dev array immediately: reloading
the rebuilt module still assembled `optimal` and reads stayed stable, but the
first 64 MiB hashed `7321098d…ed2a3` instead of the `63bf7f7d…c83673` the
fixture had been written under, because that fixture sat on the members in the
old rotating layout. Re-stamping it under the dedicated layout — 128 rows,
member 0/1 alternating, member 2 the row XOR — brought the read back to
`63bf7f7d…c83673`: the same logical bytes at a new physical placement, which is
what §3b records. Do not read that round trip as corroboration of the vendor's
write side; it only shows the driver's read path agrees with its own map.

The layout is therefore **settled against the vendor implementation** — the
arithmetic is AMD's own and no read, parity-lock or parity-memory path applies
a row term — rather than inferred from one array. What is still missing is
*corroboration on real bytes*: the staged fixture is written by this driver
(re-stamped under the dedicated layout and round-tripping cleanly), so it
cannot discriminate between candidate layouts. Two things stay unobserved
rather than unsettled: no vendor-written array has been read back, and member
order comes from the LD element array.

What is still worth doing on hardware, once such data exists, is to confirm
the *read* map end to end:

### 3a. Vendor-written data (non-destructive, preferred)

- [ ] While booted with the AMD RAIDXpert2/Windows driver (or using an array
      that already holds data), write a known pattern — e.g. a file of random
      data, or `mkfs` + files — and record its checksum.
- [ ] Boot Linux with `rcraid`, read the same range through `/dev/rcraid0`
      (or mount read-only), and compare. A clean match across several
      stripe-sized ranges is strong evidence; interleaved garbage means the
      mapping is wrong.
- [ ] Record the observed layout here: `LAYOUT = ______________`.

### 3b. Direct-member stamp (destructive; validates the driver against whichever map it implements)

This proves the driver reads the map it implements; it cannot prove the vendor
writes that map. Useful for a regression check after the map changes, and to
separate a read-path bug from a layout mismatch.

The **vendor** map for N=3, 256 KiB chunks — what a stamped fixture should
now use:

| row | parity member | data members (stripe 0, stripe 1) | physical offset on every member |
|---|---|---|---|
| 0 | 2 | 0, 1 | `UserDataOffset + 0 × chunk` |
| 1 | 2 | 0, 1 | `UserDataOffset + 1 × chunk` |
| 2 | 2 | 0, 1 | `UserDataOffset + 2 × chunk` |

Parity is fixed on the last member (`N-1`); data chunks are members `0..N-2`
in ascending order, packed at the same row offset on each member. Stamp
`data0`, `data1`, `P = data0 XOR data1` accordingly, load the driver, and
verify `/dev/rcraid0` returns the logical data. Restore zeros afterwards (the
array started zeroed).

- [x] Stamp + read-back matches (2026-11-08): 128 rows / 64 MiB stamped —
      members 0/1 alternating data chunks, member 2 the row XOR — then read
      back byte-identical through `/dev/rcraid0` (`63bf7f7d…c83673`), with the
      ext4 filesystem mounting read-only and all file checksums matching.
- [ ] Restore zeros afterwards — deferred; the array is scratch and the 64 MiB
      region still holds the fixture.

Two procedural traps, both hit on the array box:

1. `rcbottom` pins `driver_override=rcbottom` on every member it owns, so
   binding one back to `nvme` to stamp it raw fails with `EIO` until that
   attribute is cleared (`echo > /sys/bus/pci/devices/<bdf>/driver_override`).
   Restore it before reloading `rcraid`, or the members lose their binding.
2. Do not test "is the module loaded" with `lsmod | grep -q` in a script that
   sets `set -o pipefail`: grep exits on the first match, `lsmod` takes
   SIGPIPE, and the pipeline reports 141 — the test is then always false.
   Read `/sys/module/rcraid` instead. (The same bug silently disabled the
   read-only-veto check in `validate-raid5.sh` until 2026-11-08.)

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
| 3 | Parity placement | settled against the vendor code + map changed | issue #6: dedicated parity on member `N-1`, driver follows it since 2026-11-08; not yet corroborated by a vendor-written read |
| 4 | Boot / install | | blocked on writes |

## Promote

Do **not** mark RAID 5 stable until the parity write path and rebuild exist.
Placement no longer gates it — the layout is settled against the vendor code
and the map matches it; what step 3 adds is corroboration on real bytes. The
README row may already say "assembles and reads on hardware; layout settled
against the vendor implementation", and moves to stable only when writes land.
