# OS install payload on each RAID level (2026-11-14)

What this measures: writing the complete Kubuntu 26.10 root filesystem onto
`/dev/rcraid0` at each level, then verifying it. It is the same filesystem
tree the installer lays down, written through the driver's normal write path;
it is **not** a run of the graphical installer itself (see "What this is not").

## Rig

| | |
|---|---|
| Array | 4 × Sabrent Rocket Q 4 TB behind `1022:B000`, `rcbottom`, `enable_writes=1 parity_writes=1 parity_create=1` |
| Host | AMD Ryzen 7 9700X, 30 GB RAM, kernel 7.3.0-10-generic, root on USB |
| Payload | `kubuntu-26.10-desktop-amd64.iso` → `casper/filesystem.squashfs`, 4,543,128,419 bytes (xz) |
| Written | 8,469,456,619 bytes, 245,745 filesystem entries |
| GUI | rcraidXpert, same build as the repo, screenshot per level below |

## Results

| Level | Members | Capacity | Raw seq write (8 GiB, O_DIRECT) | mkfs.ext4 | Payload write (8.47 GB) | Payload rate | e2fsck -fn | Raw seq read (4 GiB) | `/usr/bin/bash` hash |
|---|---|---|---|---|---|---|---|---|---|
| RAID 0 | 4 | 14,902 GiB | 8.3 GB/s | 6.00 s | 7.41 s | 1090 MiB/s | clean, 4.24 s | 9.4 GB/s | match |
| RAID 1 | 2 | 3,725 GiB | 2.9 GB/s | 1.96 s | 6.54 s | 1235 MiB/s | clean, 1.97 s | 4.8 GB/s | match |
| RAID 10 | 4 | 7,451 GiB | 5.0 GB/s | 3.34 s | 6.05 s | 1335 MiB/s | clean, 2.30 s | 4.7 GB/s | match |
| RAID 5 | 4 | 11,177 GiB | 777 MB/s | 4.14 s | 18.93 s | 427 MiB/s | clean, 3.37 s | 3.7 GB/s | match |

Every level: `mkfs.ext4` succeeded, the tree extracted with `unsquashfs -f`
without errors, `e2fsck -fn` reported the filesystem clean, and a file read
back from the array matched the squashfs content byte for byte.

## How to read the numbers

- **Raw write/read** are 8 GiB writes of zeros and 4 GiB reads with
  `O_DIRECT`, so they bypass the page cache and measure the array path.
- **Payload rate** is `unsquashfs` extracting a 4.5 GB **xz** squashfs with
  one thread. On RAID 0/1/10 the decompressor, not the array, is the limit —
  RAID 1 and RAID 10 "outrun" their own raw write figure for that reason.
  Only the RAID 5 payload number is array-bound, and it agrees with its raw
  write number.
- **RAID 5 sequential write is the outlier**: 777 MB/s against 3.7 GB/s read,
  i.e. ~4.8× slower writes than reads, while the same four disks stripe at
  8.3 GB/s in RAID 0. The parity write path serializes per volume lock rather
  than streaming full rows at member bandwidth. Tracked in #18; not a
  correctness failure — all data verified.
- **fsck is fast on every level** because most of each volume is still
  unwritten (lazily initialised inode tables); it is not a full-media scrub.
- RAID 5 parity is only initialised for the regions actually written. The
  volume as a whole still carries uninitialised parity outside them.

## What this is not

- Not a run of the Kubuntu installer (Calamares/subiquity). No partition
  table, bootloader or `apt` stage was exercised; the filesystem was created
  directly on `/dev/rcraid0` and the root tree copied onto it. A real
  installer run needs a bootable guest whose initramfs carries `rcraid.ko`,
  because the stock Ubuntu initramfs does not.
- Not a boot test: nothing was booted from the array.
- Not a firmware-created array: every array here was stamped by this driver.

## Screenshots

Each was taken with the array freshly created and the GUI restarted, so the
tree and the state line reflect that level:

| Level | Screenshot |
|---|---|
| RAID 0 | `docs/screenshots/os-install-raid0.png` |
| RAID 1 | `docs/screenshots/os-install-raid1.png` |
| RAID 10 | `docs/screenshots/os-install-raid10.png` |
| RAID 5 | `docs/screenshots/os-install-raid5.png` |
