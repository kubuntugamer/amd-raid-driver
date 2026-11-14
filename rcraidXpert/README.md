# rcraidXpert

Qt6 graphical console for the out-of-tree `rcraid` driver. Everything it
shows and everything it changes goes through `/dev/amd_ctl0` using the ABI in
[`src/amd_raid_uapi.h`](../src/amd_raid_uapi.h); there is no simulation layer
and no private ioctl.

## What it actually does

| Action | ioctl | Notes |
|---|---|---|
| Disk/array inventory | `AMD_IOCTL_GET_HARDWARE_INVENTORY` | Any user. Populates the tree and the disk table. |
| Volume state | `AMD_IOCTL_GET_STATUS` | Any user. Drives the optimal/degraded/resyncing text. |
| Create array | `AMD_IOCTL_CREATE_ARRAY` | Root. RAID 0, 1, 10; RAID 5 only when the module was loaded with `parity_create=1`. |
| Delete array | `AMD_IOCTL_DELETE_ARRAY` | Root. Array ID 0 (the only one the driver assembles). |

Not present, on purpose: RAID 6 create (the driver has no Q write or
initialisation path), rebuild/resync buttons, verify, expand, and
spare/stripe configuration. The metadata writer chooses the stripe geometry,
so the chunk-size combo is shown disabled rather than pretending to set it.

## Requirements

1. `rcraid.ko` loaded with at least two members bound.
2. `/dev/amd_ctl0` present (mode 0644: reads are unprivileged; create/delete
   still need `CAP_SYS_ADMIN`, checked in the driver).
3. Qt6 Widgets and CMake to build.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/rcraidXpert            # inventory only
sudo ./build/rcraidXpert       # array create/delete enabled
```

`./launch-liveusb.sh` installs the Qt6 development packages for the detected
distribution, loads the module if needed, builds into `build/`, and starts the
GUI. There is no prebuilt binary in the tree.

Headless check of the inventory wiring (no display needed):

```sh
QT_QPA_PLATFORM=offscreen ./build/rcraidXpert --dump-tree
```

## Testing without the driver

With no module loaded the GUI shows `driver not loaded` and an empty disk
table, which is the honest answer but exercises almost nothing. To drive the
rest of the interface without loading `rcraid.ko` or binding a device, use the
shim in `tools/`:

```sh
./tools/run-fake.sh                        # 4 disks, 3-member RAID 5, one free
./tools/run-fake.sh --dump-tree            # the same, headless
FAKE_CTL_LEVEL=10 FAKE_CTL_MEMBERS=4 ./tools/run-fake.sh
FAKE_CTL_DEGRADED=1 ./tools/run-fake.sh    # degraded status text
FAKE_CTL_ABSENT=1 ./tools/run-fake.sh      # behave as if the module is gone
# empty controller, so create is not blocked by an assembled volume
FAKE_CTL_MEMBERS=0 sudo -E ./tools/run-fake.sh
FAKE_CTL_MEMBERS=0 FAKE_CTL_CREATE_ERRNO=95 sudo -E ./tools/run-fake.sh
```

`tools/run-fake.sh` compiles `tools/fake_ctl0.c` into `build/fake_ctl0.so` and
launches the GUI under `LD_PRELOAD`; the shim answers only
`/dev/amd_ctl0`'s four ioctls and passes every other syscall through. Nothing
is written to any disk: CREATE and DELETE just return success (or the errno
you asked for). The GUI is not linked against it, and it is not part of the
build.

Useful states to check by hand:

| Goal | How |
|---|---|
| Driver missing | run without the shim, or `FAKE_CTL_ABSENT=1` |
| Free disks selectable, members locked | default shim |
| Create path reachable | `FAKE_CTL_MEMBERS=0 sudo -E ./tools/run-fake.sh` (no volume assembled, all disks free) |
| RAID 5 create refused | `FAKE_CTL_MEMBERS=0 FAKE_CTL_CREATE_ERRNO=95 sudo -E ./tools/run-fake.sh` (a module without `parity_create=1` behaves the same) |
| Non-root create/delete message | run the shim without `sudo` |
| Degraded volume text | `FAKE_CTL_DEGRADED=1`; hover the logical drive for the tooltip |

## Using it

* **Left tree** — `Logical Drives` lists what the driver assembled (LD id,
  level, capacity, state in the tooltip); `Physical Drives` lists every bound
  member with its `disk_index`. If the module is missing the tree says so.
* **Array Creation tab** — tick `Use` on the free disks (members of an array
  cannot be ticked), pick a level, optionally type a capacity in MiB, then
  **Commit Configuration Vector**. **Discard Changes** re-reads live state.
* **Array menu** — `Refresh Inventory` re-runs the inventory ioctl;
  `Delete Selected Array...` deletes the logical drive selected in the tree.
* **Status panel** — driver reachability, disk/volume counts, whether this
  process can mutate, and the result of the last action.
* **Dashboard / S.M.A.R.T. tabs** — read the same inventory payload.

## Caveats that matter

* **RAID 5 create emits metadata only.** Data and parity are not initialised;
  zero the new block device before putting a filesystem on it. The driver
  refuses the operation entirely unless loaded with `parity_create=1`, and the
  GUI says so instead of hiding it.
* **Delete does not erase member user areas.** It rewrites their metadata so
  the disks return to the unassigned pool; old data remains on the media.
* **One array box at a time.** Create is refused while a volume is assembled,
  matching the driver. Delete first, then create.
* Create and delete are refused for non-root with an explicit message rather
  than a silent failure.
