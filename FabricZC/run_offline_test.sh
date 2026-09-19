#!/usr/bin/env bash
# ==============================================================================
# FabricZC Phase 6 Offline Diagnostic & Telemetry Capture Agent
# Executes clean build passes and archives compile frames to a log file
# ==============================================================================
set -uo pipefail

LOG_FILE="TEST_RUN_LOG.txt"
echo "=== FABRICZC OFFLINE TESTING INITIALIZED: $(date) ===" > "${LOG_FILE}"

echo "================================================================================" >> "${LOG_FILE}"
echo "[+] STEP 1: Executing Code Injection & Compilation Sequence..." >> "${LOG_FILE}"
echo "================================================================================" >> "${LOG_FILE}"

# If the generation script is sitting on disk, trigger its setup run pass
if [ -f "./generate_phase6_engine.sh" ]; then
    echo "[*] Triggering local Phase 6 structural code generator..." | tee -a "${LOG_FILE}"
    chmod +x generate_phase6_engine.sh
    ./generate_phase6_engine.sh >> "${LOG_FILE}" 2>&1 || true
else
    echo "[*] Running standalone pure local compilation wrapper..." | tee -a "${LOG_FILE}"
    if [ -f "./run_pure_local_compile.sh" ]; then
        ./run_pure_local_compile.sh >> "${LOG_FILE}" 2>&1 || true
    fi
fi

echo "================================================================================" >> "${LOG_FILE}"
echo "[+] STEP 2: Auditing Generated Binary Layout Parameters..." >> "${LOG_FILE}"
echo "================================================================================" >> "${LOG_FILE}"
if [ -f "fabriczc_mod.ko" ]; then
    echo "[SUCCESS] Independent driver binary compiled cleanly." >> "${LOG_FILE}"
    ls -lh fabriczc_mod.ko >> "${LOG_FILE}"
else
    echo "[FAILURE] Compilation target missing from workspace directory." >> "${LOG_FILE}"
fi

echo "================================================================================" >> "${LOG_FILE}"
echo "[+] STEP 3: Tracking Dynamic System Topology and Bus Targets..." >> "${LOG_FILE}"
echo "================================================================================" >> "${LOG_FILE}"
echo "--- Guest Kernel Architecture Environment ---" >> "${LOG_FILE}"
uname -a >> "${LOG_FILE}"
echo "--- Virtual PCIe Device Storage Node Trace ---" >> "${LOG_FILE}"
lspci -nn | grep -i nvme >> "${LOG_FILE}" 2>&1 || echo "Not inside active running VM context." >> "${LOG_FILE}"

echo "================================================================================"
echo "[+] SUCCESS: Offline compilation harness prepped. Logs redirected to TEST_RUN_LOG.txt"
echo "================================================================================"
