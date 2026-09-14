#!/usr/bin/env bash
echo "=== AUDITING AMD SOURCE STRINGS FOR HIDDEN ROADMAP GAPS ===" > upstream_todo_audit.txt
echo "--------------------------------------------------------" >> upstream_todo_audit.txt

echo "[*] Scanning for multi-stripe splitting blocks..."
grep -rnw src/ -e "chunk_sectors" -e "fan-out" >> upstream_todo_audit.txt || true

echo "[*] Scanning for error tracking handles..."
grep -rnw src/ -e "BLK_STS_IOERR" -e "timeout" >> upstream_todo_audit.txt || true

echo "[*] Scanning for RAID 10 architectural references..."
grep -rnw src/ -e "RAID10" -e "nested" >> upstream_todo_audit.txt || true

cat upstream_todo_audit.txt
