#!/bin/bash
# FabricZC Automated AI Context Mapping Utility
# Generates a unified structure dump for direct LLM ingestion

CONTEXT_FILE="AI_CONTEXT_DUMP.txt"
echo "=== FABRICZC LIVE ARCHITECTURE CONTEXT MAP ===" > $CONTEXT_FILE
echo "Generated on: $(date)" >> $CONTEXT_FILE
echo -e "\n--- REPOSITORY TREE STRUCTURE ---" >> $CONTEXT_FILE
tree -I ".git" 2>/dev/null || find . -not -path '*/.*' >> $CONTEXT_FILE

echo -e "\n--- GLOBAL STRUCTURE DEFINITIONS (fabriczc.h) ---" >> $CONTEXT_FILE
cat include/fabriczc.h >> $CONTEXT_FILE

echo -e "\n--- TARGET SYSTEM INSTRUCTIONS ---" >> $CONTEXT_FILE
cat AI_SPEC.md >> $CONTEXT_FILE

echo ">>> AI Context Dump successfully generated at: $CONTEXT_FILE <<<"
