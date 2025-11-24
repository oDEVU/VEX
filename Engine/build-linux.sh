#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

echo "  ===  STARTING FULL ENGINE BUILD  ==="

echo "[1/2] Entering Core..."
cd Core
./rebuild-core-linux.sh
cd ..
echo "Core build successful."

echo "[2/2] Entering BuildTools..."
cd BuildTools
./rebuild-buildtools-linux.sh
cd ..
echo "BuildTools build successful."

echo "  ===  ALL MODULES BUILT SUCCESSFULLY  ==="
