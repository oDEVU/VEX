#!/bin/bash
set -e

BUILD_DIR=${1:-build}

echo "Building ProjectBuilder into: $BUILD_DIR"

cmake -B "$BUILD_DIR" -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build "$BUILD_DIR"

echo "Creating global commands (Sudo password required)..."
TOOLS=("VPAK_Packer" "ProjectBuilder" "VexCrashDecoder")
ABS_BUILD_DIR=$(cd "$BUILD_DIR" && pwd)

for tool in "${TOOLS[@]}"; do
    TARGET="$ABS_BUILD_DIR/$tool"
    LINK_NAME="/usr/local/bin/${tool,,}"

    if sudo ln -sf "$TARGET" "$LINK_NAME"; then
        echo "    [+] Command '$LINK_NAME' created."
    else
        echo "    [-] Failed to create '$LINK_NAME'."
    fi
done

echo "Setup complete. You can now run the tools (e.g., 'vexcrashdecoder') from any terminal."
