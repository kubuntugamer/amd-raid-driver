#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

echo "🛠️  Step 1: Compiling shared object plugin library..."
make clean
make

echo "📝 Step 2: Generating a valid VirtualBox format uppercase SHA-256 manifest..."
echo "dummy" > ExtPack.signature

# Force uppercase conversion and align paths with exactly two spaces
sha256sum ExtPack.xml | awk '{print toupper($1) "  ExtPack.xml"}' > ExtPack.manifest
sha256sum ExtPack.signature | awk '{print toupper($1) "  ExtPack.signature"}' >> ExtPack.manifest
sha256sum my_plugin.cpp | awk '{print toupper($1) "  my_plugin.cpp"}' >> ExtPack.manifest
sha256sum linux.amd64/libmy_plugin.so | awk '{print toupper($1) "  linux.amd64/libmy_plugin.so"}' >> ExtPack.manifest

echo "🚀 Step 3: Compressing directory tree locally into a portable Extension Pack archive..."
PACK_NAME="AMD-RCRAID-Emulator-Pack-7.2.18.vbox-extpack"
tar -cvzf "${PACK_NAME}" \
    ExtPack.xml \
    ExtPack.signature \
    ExtPack.manifest \
    my_plugin.cpp \
    linux.amd64/libmy_plugin.so

echo "✅ SUCCESS! Distributable package created locally: 1022-b000/${PACK_NAME}"
