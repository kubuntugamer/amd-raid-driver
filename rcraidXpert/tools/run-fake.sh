#!/bin/bash
# Run rcraidXpert against a fake /dev/amd_ctl0 (no module, no PCI binding).
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(cd "$here/.." && pwd)
build_dir=${BUILD_DIR:-$root/build}
shim=$build_dir/fake_ctl0.so

if [ ! -x "$build_dir/rcraidXpert" ]; then
    cmake -S "$root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$build_dir" -j"$(nproc)"
fi

mkdir -p "$build_dir"
gcc -shared -fPIC -O2 -I"$root/../src" -o "$shim" "$here/fake_ctl0.c" -ldl

FAKE_CTL_DISKS=${FAKE_CTL_DISKS:-4}
FAKE_CTL_MEMBERS=${FAKE_CTL_MEMBERS:-3}
FAKE_CTL_LEVEL=${FAKE_CTL_LEVEL:-5}

export FAKE_CTL_DISKS FAKE_CTL_MEMBERS FAKE_CTL_LEVEL
exec env LD_PRELOAD="$shim" "$build_dir/rcraidXpert" "$@"
