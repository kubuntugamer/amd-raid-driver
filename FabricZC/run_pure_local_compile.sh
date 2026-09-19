#!/usr/bin/env bash
set -euo pipefail
gcc -nostdinc -std=gnu11 \
    -I"$(pwd)/staging_includes" \
    -D__KERNEL__ \
    -DMODULE \
    -O2 -Wall -m64 -mcmodel=kernel -mno-red-zone -fno-pie \
    -c src/kernel/main.c -o src/kernel/main.o

gcc -nostdinc -std=gnu11 \
    -I"$(pwd)/staging_includes" \
    -D__KERNEL__ \
    -DMODULE \
    -O2 -m64 -mcmodel=kernel -mno-red-zone -fno-pie \
    -c src/kernel/fabriczc_mod.mod.c -o src/kernel/fabriczc_mod.mod.o

ld -r -m elf_x86_64 -z max-page-size=0x1000 \
    -o fabriczc_mod.ko \
    src/kernel/main.o \
    src/kernel/fabriczc_mod.mod.o
