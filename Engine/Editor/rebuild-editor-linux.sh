#!/bin/bash
set -e

echo "=== Building Editor ==="
echo "Configuring Editor:"
cmake -G Ninja -S . -B ./bin -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
echo "Compiling Editor:"
cmake --build ./bin --config Debug --parallel

echo "=== Editor Build Process Completed ==="

echo ""
read -p "Install Vex Engine system-wide? (Global CLI commands + App Menu Shortcut) [y/N] " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Installing..."

    mkdir -p ~/.local/share/applications
    cp ./bin/VexEngine.desktop ~/.local/share/applications/
    update-desktop-database ~/.local/share/applications
    echo "'Vex Engine' shortcut added to App Menu."

    TOOLS=("VexEditor" "VexProjectSelector")

    echo "Creating global commands (Sudo password required)..."
    for tool in "${TOOLS[@]}"; do
        TARGET="$(pwd)/bin/$tool"
        LINK_NAME="/usr/local/bin/${tool,,}"

        if sudo ln -sf "$TARGET" "$LINK_NAME"; then
            echo "    [+] Command '$LINK_NAME' created."
        else
            echo "    [-] Failed to create '$LINK_NAME'."
        fi
    done

    echo "You can now run 'vexeditor' or 'vexprojectselector' from any terminal."
else
    echo "Skipping installation."
fi
