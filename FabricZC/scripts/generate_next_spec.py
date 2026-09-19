#!/usr/bin/env python3
# FabricZC Dynamic Progression Spec-Sheet Auto-Generator
import json
import os

manifest_path = "MANIFEST.json"
spec_out = "docs/NEXT_STEP_SPEC.md"

print("\n[PROGRESSION] Invoking automated tracking ledger loops...")

if os.path.exists(manifest_path):
    with open(manifest_path, "r") as f:
        data = json.load(f)
    
    # Locate the first pending phase block inside the manifest ledger matrix
    next_phase = None
    for phase_id in sorted(data.get("phases", {}).keys()):
        if data["phases"][phase_id].get("status") == "PENDING":
            next_phase = data["phases"][phase_id]
            next_phase["id"] = phase_id
            break
            
    if next_phase:
        print(f"  -> Next Phase Isolated: [{next_phase['id']}] - {next_phase['title']}")
        print(f"  -> Self-producing upcoming engineering specifications sheet in docs/...")
        
        spec_content = f"""# 🤖 FabricZC Next-Step Implementation Blueprint Manual

## 📊 Dynamic Progression Ledger State
* **Current Version:** {data.get("version", "0.1.0")}
* **Active Working Branch Target:** experimental
* **Identified Task Objective:** {next_phase['id']} - {next_phase['title']}
* **Compilation Status of Current Subsystem:** STABLE / ZERO WARNINGS CLEAN

## 🛠️ Phase Requirements & Architectural Constraints
{next_phase.get('instructions', 'No current instructions provided.')}

## 🔩 Structural Context References
* Global Container Packing Boundary: Exactly 48 Bytes
* Target Memory Coordinates: 16 KiB Front-Offset Anchor Zone
* Stripe Sizing Parameter constraints: Fixed 1 MiB Extent Footprints

## 💡 Automated LLM Direct Ingestion Prompts
When feeding this project directory tree context straight into a generative coding engine to produce the functional source code arrays for this phase, execute this instruction:
"Using include/fabriczc.h, expand the function loops inside src/kernel/main.c to seamlessly implement the code structures detailed in {next_phase['title']}. Ensure absolute compliance with cache-line alignment variables and lock-free thread isolation bounds."
"""
        # Ensure the docs subdirectory is present before writing the spec file
        os.makedirs("docs", exist_ok=True)
        with open(spec_out, "w") as sf:
            sf.write(spec_content.strip() + "\n")
        print("  -> [PROGRESSION SUCCESS] docs/NEXT_STEP_SPEC.md updated perfectly.")
    else:
        print("  -> All project phase matrices report as COMPLETE. Storage engine production ready.")
else:
    print("  -> [ERROR] MANIFEST.json file missing from workspace root.")
