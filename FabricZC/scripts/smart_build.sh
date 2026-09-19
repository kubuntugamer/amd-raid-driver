#!/bin/bash
# FabricZC Self-Sustaining Production Script
# Automatically traps, parses, and self-corrects compilation syntax breaks on the fly

WORKSPACE=$(pwd)
LOG_DUMP="/tmp/fzc_build_error.log"

echo "=== 🤖 INITIATING AUTONOMOUS SMART-BUILD CONTEXT ENGINE ==="

run_build_pass() {
    echo "[STAGE] Executing compilation loop pass..."
    # Execute make, merge stdout and stderr, and dump any errors directly into our log tracker
    make > "$LOG_DUMP" 2>&1
    return $?
}

# Run the initial compilation check pass
run_build_pass
STATUS=$?

# If the status code is non-zero, an error took place. Enter the self-correction loop!
while [ $STATUS -ne 0 ]; do
    echo -e "\033[1;33m[ALERT]\033[0m Compilation break intercepted (Exit Code: $STATUS). Auditing error logs..."
    cat "$LOG_DUMP" | grep -iE "error:|warning:" || true

    # HEURISTIC PATTERN 1: Detect unescaped double-quote syntax breaks inside print loops
    if grep -q "expected ')' before '!' token" "$LOG_DUMP"; then
        echo -e "  -> \033[1;32m[HEAL]\033[0m Isolated escape character quoting clash inside src/user/control.c."
        echo "  -> Applying automated print statement string formatting patch..."
        
        # Use an automated regex replacement pass to cleanly wrap macro tokens in valid literal print quotes
        sed -i 's/("!ZCH")/("\\"!ZCH\\\"")/g' src/user/control.c
        sed -i 's/("!ZCD")/("\\"!ZCD\\\"")/g' src/user/control.c
        
        # Mirror the clean-string corrections directly back into the master blueprint.sh template file
        sed -i 's/("!ZCH")/(\\\\\"!ZCH\\\\\")/g' blueprint.sh 2>/dev/null || true
        sed -i 's/("!ZCD")/(\\\\\"!ZCD\\\\\")/g' blueprint.sh 2>/dev/null || true
    
    # HEURISTIC PATTERN 2: Detect unescaped backslashes and stray character symbols
    elif grep -q "stray '\\\\' in program" "$LOG_DUMP"; then
        echo -e "  -> \033[1;32m[HEAL]\033[0m Isolated stray backslash sequence contamination."
        echo "  -> Stripping double-escape formatting anomalies from source files..."
        sed -i 's/\\\\"!/\\"!/g' src/user/control.c
        sed -i 's/!\\\\"/!\\"/g' src/user/control.c

    # DEFAULT FALLBACK: Unrecognized error profile encountered
    else
        echo -e "\033[1;31m[CRITICAL]\033[0m Unrecognized error pattern. Manual architectural intervention required."
        echo "Exiting autonomous healing loop to protect file integrity paths."
        exit 1
    fi

    # Re-trigger a fresh build pass to check if the applied code patches successfully resolved the syntax break
    echo "[STAGE] Re-verifying patched source files..."
    run_build_pass
    STATUS=$?
done

echo -e "\n\033[1;32m[🎉 SUCCESS]\033[0m FabricZC workspace compiled flawlessly with zero remaining syntax breaks!"
echo "Target Executable Map: $(ls -l src/user/fabriczc_ctl)"
