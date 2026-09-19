#!/usr/bin/env bash
# ==============================================================================
# FabricZC Automated Documentation Synchronization Hook
# Evaluates MANIFEST.json and automatically updates README.md specifications
# ==============================================================================

set -euo pipefail

# Ensure active path mapping is focused cleanly inside the sandbox directory
if [ -d "FabricZC" ] && [ ! -f "MANIFEST.json" ]; then
    cd FabricZC
fi

echo "[*] Parsing sandbox project progression parameters..."
CURRENT_STATUS=$(grep '"status":' MANIFEST.json | awk -F'"' '{print $4}' || echo "UNKNOWN")

echo "[+] Detected Development State: ${CURRENT_STATUS}"

# If the self-healing engine logs confirm a state update, rewrite the prompt manual hooks
if [ "${CURRENT_STATUS}" = "PHASE_4_STAGED" ]; then
    echo "[+] Synchronizing Phase 5 AI Core Ingestion Prompts inside README.md..."
    
    # Trigger an automated local write pass to bump version markers and update prompts
    sed -i 's/\* \*\*Phase 4 (SGL Translation Matrix):\*\* PENDING/\* \*\*Phase 4 (SGL Translation Matrix):\*\* COMPLETE/g' README.md || true
    sed -i 's/\* \*\*Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping):\*\* PENDING/\* \*\*Phase 5 (Direct-to-Disk Linear Extent Stripe Mapping):\*\* PENDING \/ ACTIVE NEXT TASK/g' README.md || true
    
    echo "[+] Documentation automated synchronization pass finalized clean."
fi
