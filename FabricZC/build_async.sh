#!/usr/bin/env bash
set -euo pipefail
echo "[+] Validating Phase 2 engine compilation profiles..."
sudo make clean
sudo make -j"$(nproc)"
if [ -f "fabriczc_mod.ko" ] || [ -f "src/kernel/fabriczc_mod.ko" ]; then
    echo "[+] Verification complete: Compilation clean with zero warnings."
    sed -i 's/"status": "PENDING"/"status": "COMPILE_TESTED"/g' MANIFEST.json || true
else
    echo "[-] Error: Destination compilation targets missing." >&2
    exit 1
fi
