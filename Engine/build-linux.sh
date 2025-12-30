#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

if [ -d "Core/bin/Distribution" ]; then
    echo "[Setup] Cleaning old Distribution binaries..."
    rm -rf "Core/bin/Distribution"
fi

if [ "$1" == "-clean" ]; then
    echo "[Clean] Removing ALL Engine Binaries..."
    rm -rf Core/bin
    rm -rf BuildTools/build
    rm -rf Editor/bin
fi

echo "  ===  STARTING FULL ENGINE BUILD  ==="

echo "[1/3] Entering Core..."
cd Core
./rebuild-core-linux.sh
cd ..
echo "Core build successful."

echo "[2/3] Entering BuildTools..."
cd BuildTools
./rebuild-buildtools-linux.sh
cd ..
echo "BuildTools build successful."

echo "[3/3] Entering Editor..."
cd Editor
./rebuild-editor-linux.sh
cd ..
echo "Editor build successful."

echo "  ===  ALL MODULES BUILT SUCCESSFULLY  ==="
