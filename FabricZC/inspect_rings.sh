#!/usr/bin/env bash
# ==============================================================================
# FabricZC Asynchronous Scheduling Diagnostics and Thread Inspector
# Tracks core mapping parameters and real-time execution states for QA verification
# ==============================================================================

set -euo pipefail

echo "================================================================================"
echo "FABRICZC LIVE KERNEL ARCHITECTURE THREAD DIAGNOSTIC"
echo "================================================================================"

# Verify if the storage engine module is actually resident inside kernel memory spaces
if ! lsmod | grep -q "fabriczc_mod"; then
    echo "[!] Warning: fabriczc_mod is not currently loaded into memory space."
    echo "[*] Attempting to trace existing worker task configurations anyway..."
    echo "================================================================================"
fi

# Locate the precise Process IDs (PIDs) assigned to the thread pool allocations
THREAD_PATTERN="kfabriczc/"
MATCHING_TASKS=$(ps -ef | grep "${THREAD_PATTERN}" | grep -v "grep" || true)

if [ -z "${MATCHING_TASKS}" ]; then
    echo "[-] Operational Failure: No active 'kfabriczc' worker threads found."
    echo "[*] Run './build_async.sh' first to compile and introduce the kernel module."
    exit 0
fi

echo "[+] Discovered Active Allocation Group Workers:"
echo "--------------------------------------------------------------------------------"
printf "%-10s %-8s %-8s %-6s %-12s %s\n" "USER" "PID" "PPID" "C" "STIME" "COMMAND"
echo "${MATCHING_TASKS}" | while read -r line; do
    printf "%s\n" "${line}"
done

echo "--------------------------------------------------------------------------------"
echo "[+] Profiling Dynamic Kernel Thread Affinity & Scheduling Groups:"
echo "--------------------------------------------------------------------------------"

# Walk through every detected kernel task to expose core-binding and hardware residency
echo "${MATCHING_TASKS}" | awk '{print $2}' | while read -r thread_pid; do
    if [ -d "/proc/${thread_pid}" ]; then
        thread_name=$(cat "/proc/${thread_pid}/comm")
        assigned_cpu=$(awk '/processor/ {print $3}' "/proc/${thread_pid}/stat" 2>/dev/null || stat -c "%C" "/proc/${thread_pid}" 2>/dev/null || echo "N/A")
        execution_state=$(awk '{print $3}' "/proc/${thread_pid}/stat" 2>/dev/null || echo "Unknown")
        
        echo " -> Worker PID [${thread_pid}] (${thread_name}) | Core Affinity Slot: ${assigned_cpu} | State: ${execution_state}"
    fi
done

echo "================================================================================"
echo "[+] Streaming Real-Time FabricZC Ring Kernel Ring Buffers (dmesg):"
echo "================================================================================"
sudo dmesg | grep -i "FabricZC" | tail -n 10
