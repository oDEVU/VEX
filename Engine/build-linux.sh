#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

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
