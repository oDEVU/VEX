#!/bin/bash
set -e

BUILD_DIR=${1:-build}

echo "Building ProjectBuilder into: $BUILD_DIR"

cmake -B "$BUILD_DIR" -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build "$BUILD_DIR"
