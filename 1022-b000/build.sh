#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

echo "🛠️  Step 1: Compiling shared object plugin library..."
make clean
make

echo "📝 Step 2: Generating a valid, standard SHA-256 verification manifest..."
echo "dummy" > ExtPack.signature

# VirtualBox requires a strict two-space sha256sum structure tracking EVERY file in the archive
# If any file is missing from this list, the VirtualBox GUI Installer throws a VERR_PARSE_ERROR
sha256sum ExtPack.xml > ExtPack.manifest
sha256sum ExtPack.signature >> ExtPack.manifest
sha256sum my_plugin.cpp >> ExtPack.manifest
sha256sum linux.amd64/libmy_plugin.so >> ExtPack.manifest

echo "🚀 Step 3: Compressing directory tree locally into a portable Extension Pack archive..."
PACK_NAME="AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack"

# Use the exact structural file parameters declared inside the manifest
tar -cvzf "${PACK_NAME}" \
    ExtPack.xml \
    ExtPack.signature \
    ExtPack.manifest \
    my_plugin.cpp \
    linux.amd64/libmy_plugin.so

echo "✅ SUCCESS! GUI-Ready portable package created locally: 1022-b000/${PACK_NAME}"
