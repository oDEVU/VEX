#!/bin/bash
set -e

echo "=== Building Core Engine in Debug config ==="
echo "Configuring Core for Debug:"
cmake -G Ninja -S . -B ./bin/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
echo "Compiling Core for Debug:"
cmake --build ./bin/Debug --config Debug --parallel

echo "=== Building Core Engine in Release config ==="
echo "Configuring Core for Release:"
cmake -G Ninja -S . -B ./bin/Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
echo "Compiling Core for Release:"
cmake --build ./bin/Release --config Release --parallel

echo "=== Core Build Process Completed ==="
