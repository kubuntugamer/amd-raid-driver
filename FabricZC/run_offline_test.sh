#!/usr/bin/env bash
# ==============================================================================
# FabricZC Offline Diagnostics & Telemetry Logging Tool
# Captures compiler states and simulation logs into a persistent file
# ==============================================================================
set -uo pipefail

LOG_FILE="TEST_RUN_LOG.txt"
echo "=== FABRICZC OFFLINE TESTING INITIALIZED: $(date) ===" > "${LOG_FILE}"

echo "[+] STEP 1: Building Storage Module Target Matrix..." | tee -a "${LOG_FILE}"
if [ -f "./run_pure_local_compile.sh" ]; then
    ./run_pure_local_compile.sh >> "${LOG_FILE}" 2>&1 || true
fi

echo "[+] STEP 2: Auditing Generated Binary Layout Parameters..." | tee -a "${LOG_FILE}"
if [ -f "fabriczc_mod.ko" ]; then
    echo "[SUCCESS] fabriczc_mod.ko artifact exists." >> "${LOG_FILE}"
    ls -lh fabriczc_mod.ko >> "${LOG_FILE}"
else
    echo "[FAILURE] Compilation output missing." >> "${LOG_FILE}"
fi

echo "[+] STEP 3: Tracking Dynamic System Topology..." | tee -a "${LOG_FILE}"
echo "--- Guest Kernel Architecture Environment ---" >> "${LOG_FILE}"
uname -a >> "${LOG_FILE}"
echo "--- Virtual PCIe Device Storage Node Trace ---" >> "${LOG_FILE}"
lspci -nn | grep -i nvme >> "${LOG_FILE}" 2>&1 || echo "Not inside active running VM context." >> "${LOG_FILE}"

echo "================================================================================" | tee -a "${LOG_FILE}"
echo "[+] SUCCESS: Telemetry saved cleanly to FabricZC/TEST_RUN_LOG.txt" | tee -a "${LOG_FILE}"
echo "================================================================================" | tee -a "${LOG_FILE}"
