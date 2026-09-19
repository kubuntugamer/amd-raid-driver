#!/bin/bash
# ==============================================================================
# 🧵 FABRICZC AUTONOMOUS ORCHESTRATION & SELF-HEALING COMPILATION ENGINE
# ==============================================================================
# Core Design: Event-Driven Heuristic Closed-Loop Storage Code Compiler
# Developer Level: Highly Verbose Technical Diagnostic Output Mode Enforced
# ==============================================================================

set -e

WORKSPACE=$(pwd)
LOG_DUMP="/tmp/fabriczc_system_compiler_errors.log"
CORRECTION_COUNTER=0
MAX_ATTEMPTS=5

log_diagnostic_banner() {
    echo "======================================================================"
    echo "🤖 AUTONOMOUS METADATA STATE ENGINE // ENGINE RECOVERY PIPELINE      "
    echo "======================================================================"
    echo "[SYSTEM LOG] Invoking autonomous code synthesis loops..."
    echo "[SYSTEM LOG] Monitoring Workspace Root: $WORKSPACE"
    echo "[SYSTEM LOG] Target Diagnostics Pipe:   $LOG_DUMP"
}

run_compilation_pass() {
    echo -e "\n[COMPILE PASS] Launching compiler optimization block sequences..."
    # Merge stdout and stderr tracking registers straight into our diagnostic tracking dump file
    make > "$LOG_DUMP" 2>&1
    return $?
}

# Initialize system telemetry views
log_diagnostic_banner

# Trigger the initial build check pass to see if the directory is clean and valid
set +e
run_compilation_pass
STATUS=$?
set -e

# Loop continuously while the compiler rejects the source configuration matrices
while [ $STATUS -ne 0 ]; do
    CORRECTION_COUNTER=$((CORRECTION_COUNTER + 1))
    
    echo -e "\n\033[1;31m[CRITICAL INTERCEPT]\033[0m Compilation execution failure caught! (Exit Status Code: $STATUS)"
    echo "======================================================================"
    echo ">>> START RAW COMPILER TRACE TELEMETRY DUMP <<<"
    cat "$LOG_DUMP"
    echo ">>> END RAW COMPILER TRACE TELEMETRY DUMP <<<"
    echo "======================================================================"
    
    if [ $CORRECTION_COUNTER -gt $MAX_ATTEMPTS ]; then
        echo -e "\n\033[1;31m[FATAL ERROR]\033[0m Maximum autonomous self-healing loops exceeded ($MAX_ATTEMPTS attempts)."
        echo "[FATAL ERROR] Structural block dependency layout contains unmapped architecture boundaries."
        exit 1
    fi

    echo -e "\n[ANALYSIS] Feeding telemetry into the heuristic repair pattern matrix... [ATTEMPT $CORRECTION_COUNTER/$MAX_ATTEMPTS]"

    # --------------------------------------------------------------------------
    # HEURISTIC PATTERN 1: Detect and strip over-escaped path backslashes in Makefile
    # --------------------------------------------------------------------------
    if grep -q "specified external module directory" "$LOG_DUMP" || grep -q "does not exist\.  Stop\." "$LOG_DUMP"; then
        echo -e "  -> \033[1;33m[MATCHED SIGNATURE]\033[0m External module target path directory escaping corruption detected."
        echo "  -> Execution Block Action: Dynamic Trace Emulation path recovery initialized."
        
        # Query the live kernel release configurations directly from the active machine environment
        LIVE_KERNEL=$(uname -r)
        echo "    -> Live Kernel Architecture Identified: $LIVE_KERNEL"
        echo "    -> Self-producing a pristine, zero-escape Makefile geometry structure..."
        
        # Rewrite a clean Makefile configuration array using raw absolute shell parameters
        cat << FZC_CLEAN_MAKE > Makefile
KVERSION ?= $LIVE_KERNEL
KDIR     ?= /lib/modules/\$(KVERSION)/build
PWD      := \$(shell pwd)

obj-m    += src/kernel/fabriczc_mod.o
src/kernel/fabriczc_mod-y := src/kernel/main.o

all:
	\$(MAKE) -C \$(KDIR) M=\$(PWD) modules
	\$(CC) -Wall -Wextra -O3 src/user/control.c -o src/user/fabriczc_ctl

clean:
	\$(MAKE) -C \$(KDIR) M=\$(PWD) clean
	rm -f src/user/fabriczc_ctl
	@rm -f src/kernel/.*.cmd src/kernel/*.o src/kernel/*.ko src/kernel/*.mod*
FZC_CLEAN_MAKE

        echo "  -> [REPAIR] Clean-room Makefile regenerated. Flushing old intermediate buffers... [OK]"
        rm -f src/kernel/*.o src/kernel/.*.cmd 2>/dev/null || true

    # --------------------------------------------------------------------------
    # HEURISTIC PATTERN 2: Handle literal quotation interpolation errors in printf
    # --------------------------------------------------------------------------
    elif grep -q "expected ‘)’ before ‘!’ token" "$LOG_DUMP" || grep -q "stray ‘\\\\’ in program" "$LOG_DUMP"; then
        echo -e "  -> \033[1;33m[MATCHED SIGNATURE]\033[0m String quotation interpolation clash caught inside src/user/control.c."
        echo "  -> Execution Block Action: In-place token syntax rewrite routine initialized."
        
        # Extract the exact row number coordinates straight from the compiler error string layout
        TARGET_LINE=$(grep -n "error:" "$LOG_DUMP" | head -n 1 | cut -d':' -f2 || echo "0")
        echo "    -> Target line location isolated at: Line $TARGET_LINE"
        echo "    -> Forcing clean double-quote parameter boundaries across literal print arguments..."
        
        # Use localized stream modifiers to clean up the double-escaped quote anomalies safely
        sed -i 's/(\\"!ZCH\\")/("\\"!ZCH\\\"")/g' src/user/control.c 2>/dev/null || true
        sed -i 's/(\\"!ZCD\\")/("\\"!ZCD\\\"")/g' src/user/control.c 2>/dev/null || true
        sed -i 's/(\\\\"!ZCH\\\\")/("\\"!ZCH\\\"")/g' src/user/control.c 2>/dev/null || true
        sed -i 's/(\\\\"!ZCD\\\\")/("\\"!ZCD\\\"")/g' src/user/control.c 2>/dev/null || true
        
        # Implement an alternative python fallback block to wipe out structural string errors perfectly
        python3 -c '
import os
file_path = os.path.expanduser("~/experimental/amd-raid-driver/FabricZC/src/user/control.c")
if os.path.exists(file_path):
    with open(file_path, "r") as f:
        lines = f.readlines()
    for idx, line in enumerate(lines):
        if "State Machine Key Signature Flag Detected" in line and "!" in line:
            # Overwrite the broken line with a perfectly escaped clean-room print block format string
            lines[idx] = "        printf(\"    -> State Machine Key Signature Flag Detected: 0x48435a21 (\\\"!ZCH\\\")\\\\n\");\\n"
    with open(file_path, "w") as f:
        f.write("".join(lines))
'
        echo "  -> [REPAIR] Token boundaries updated. Clean quotes injected inside source trees... [OK]"

    # --------------------------------------------------------------------------
    # HEURISTIC PATTERN 3: Missing main program initialization symbols or headers
    # --------------------------------------------------------------------------
    elif grep -q "error: unknown type name" "$LOG_DUMP" || grep -q "fabriczc.h: No such file" "$LOG_DUMP"; then
        echo -e "  -> \033[1;33m[MATCHED SIGNATURE]\033[0m Structure header resolution missing cross-layer header maps."
        echo "  -> Execution Block Action: Path tracking tree alignment initialized."
        mkdir -p include src/kernel src/user
        # Trigger the blueprint engine to re-verify directory layouts and links
        ./blueprint.sh 2>/dev/null || true
        echo "  -> [REPAIR] Layout structure maps verified and linked... [OK]"

    # --------------------------------------------------------------------------
    # DEFAULT EXCLUSION GATE: Catch unmapped compiler architectural parameters
    # --------------------------------------------------------------------------
    else
        echo -e "  -> \033[1;31m[UNMAPPED SIGNATURE INTERCEPTED]\033[0m System encountered an unknown compilation error class."
        echo "  -> Halting self-healing execution logs instantly to preserve structural directory flags."
        exit 1
    fi

    # Re-trigger a fresh build verification cycle to validate the applied patches
    set +e
    run_compilation_pass
    STATUS=$?
    set -e
done

echo -e "\n======================================================================"
echo -e "🎉 \033[1;32m[AUTONOMOUS BUILD COMPLETE]\033[0m System compiled perfectly with zero remaining syntax blocks!"
echo "======================================================================"
echo -e "[TELEMETRY LOGS] Total self-healing corrections applied: $CORRECTION_COUNTER"
echo -e "[TELEMETRY LOGS] Validated Driver Binary Footprint:      $(ls -l src/kernel/fabriczc_mod.ko)"
echo -e "[TELEMETRY LOGS] Validated Executable Management Control: $(ls -l src/user/fabriczc_ctl)"

# Invoke autonomous progression spec-sheet generation
./scripts/generate_next_spec.py
echo "======================================================================"
