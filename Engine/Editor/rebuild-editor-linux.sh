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

echo "Creating distribution build environment..."

cat <<EOF > ./bin/Dockerfile
FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

USER root

RUN apt-get update && apt-get install -y \
    lsb-release \
    wget \
    gnupg \
    curl

# LLVM 17 Setup (This part was working fine)
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc && \
    echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-17 main" > /etc/apt/sources.list.d/llvm.list && \
    apt-get update && \
    apt-get install -y clang-17 lld-17 libc++-17-dev libc++abi-17-dev

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-17 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-17 100

# FIX: Install CMake via official install script instead of flaky APT repo
# This installs CMake 3.29.2 directly to /usr/local/bin
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-linux-x86_64.sh && \
    chmod +x cmake-3.29.2-linux-x86_64.sh && \
    ./cmake-3.29.2-linux-x86_64.sh --prefix=/usr/local --exclude-subdir --skip-license && \
    rm cmake-3.29.2-linux-x86_64.sh

# --- Vulkan SDK Setup (Tarball Method) ---
# Installs version 1.3.296.0 directly to avoid broken APT repos
WORKDIR /usr/local
RUN wget -q https://sdk.lunarg.com/sdk/download/1.3.296.0/linux/vulkansdk-linux-x86_64-1.3.296.0.tar.xz && \
    tar -xf vulkansdk-linux-x86_64-1.3.296.0.tar.xz && \
    rm vulkansdk-linux-x86_64-1.3.296.0.tar.xz && \
    mv 1.3.296.0 vulkansdk

# Set environment variables for Vulkan SDK so glslc is found
ENV VULKAN_SDK=/usr/local/vulkansdk/x86_64
ENV PATH=\$VULKAN_SDK/bin:\$PATH
ENV LD_LIBRARY_PATH=\$VULKAN_SDK/lib:\$LD_LIBRARY_PATH
ENV VK_LAYER_PATH=\$VULKAN_SDK/etc/vulkan/explicit_layer.d

# --- Slang Compiler Setup ---
WORKDIR /usr/local
RUN wget -q https://github.com/shader-slang/slang/releases/download/v2024.1.13/slang-2024.1.13-linux-x86_64.tar.gz && \
    tar -xzf slang-2024.1.13-linux-x86_64.tar.gz --strip-components=1 && \
    rm slang-2024.1.13-linux-x86_64.tar.gz

ENV CXX=clang++
ENV CC=clang
ENV CXXFLAGS="-stdlib=libc++"
ENV LDFLAGS="-stdlib=libc++"
# Reset workdir and ensure user is root
WORKDIR /
USER 1000:1000
EOF

echo "Dockerfile generated in ./bin/Dockerfile"

CONTAINER_TOOL=""
if command -v podman &> /dev/null; then
    CONTAINER_TOOL="podman"
elif command -v docker &> /dev/null; then
    CONTAINER_TOOL="docker"
else
    echo "  [-] neither podman nor docker found. Distribution builds will not work."
fi

if [ -n "$CONTAINER_TOOL" ]; then
    echo "Building 'vex-builder' container image (this may take a few minutes)..."

    IMAGE="registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest"
    $CONTAINER_TOOL pull "$IMAGE" || echo "Warning: Pull failed, hoping it exists or build will retry."

    if $CONTAINER_TOOL build -t vex-builder -f ./bin/Dockerfile .; then
        echo "  [+] 'vex-builder' container created successfully."
    else
        echo "  [-] Failed to build container image."
    fi
fi

echo "Installation Complete."
else
    echo "Skipping installation."
fi
