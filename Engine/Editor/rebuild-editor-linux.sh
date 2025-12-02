#!/bin/bash
set -e

echo "=== Building Editor ==="
echo "Configuring Editor:"
cmake -G Ninja -S . -B ./bin -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
echo "Compiling Editor:"
cmake --build ./bin --config Debug --parallel

echo "=== Editor Build Process Completed ==="
