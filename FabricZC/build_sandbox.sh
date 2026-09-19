#!/usr/bin/env bash
# ==============================================================================
# FabricZC Overriding Isolated Sandbox Compilation Tool
# Forces local header inclusions to bypass external Kconfig dependency faults
# ==============================================================================
set -euo pipefail

echo "================================================================================"
echo "[+] STEP 1: Verifying Local Parameter Overrides inside Sandbox..."
echo "================================================================================"
echo "[*] Active Target Working Path: $(pwd)"

# Define explicit localized include directories
LOCAL_INC="$(pwd)/include"

# Clear out any residual build maps from prior aborted passes
make clean

echo "================================================================================"
echo "[+] STEP 2: Invoking Kernel Build Module with Isolated Flag Strings..."
echo "================================================================================"

# Execute the kernel build interface while explicitly binding preprocessor variables
# This bypasses the global autoconf dependency by feeding an explicit local include link
make -j"$(nproc)" \
  EXTRA_CFLAGS="-I${LOCAL_INC} -D__KERNEL__" \
  KCFLAGS="-I${LOCAL_INC}" \
  NOSTDINC_FLAGS="-I${LOCAL_INC}"

echo "================================================================================"
echo "[+] STEP 3: Tracking Output Target Structural States..."
echo "================================================================================"

if [ -f "fabriczc_mod.ko" ] || [ -f "src/kernel/fabriczc_mod.ko" ] || [ -f "modules.order" ]; then
    echo "[+] SUCCESS: FabricZC source files compiled cleanly using isolated include flags."
    
    # Generate the subsequent runtime configuration validation file script node
    cat << 'INNER_EOF' > verify_local_binary.sh
#!/usr/bin/env bash
set -euo pipefail
echo "=== FabricZC Internal Binary Signature Audit ==="
modinfo fabriczc_mod.ko || modinfo src/kernel/fabriczc_mod.ko
INNER_EOF
    chmod +x verify_local_binary.sh
    echo "[+] Next validation tracking block written to: ./verify_local_binary.sh"
else
    echo "[-] Error: Compilation bypass block finished but binary target was not found." >&2
    exit 1
fi
