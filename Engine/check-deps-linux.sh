#!/bin/bash

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian|pop|mint) echo "debian" ;;
            fedora|rhel|centos) echo "fedora" ;;
            arch|manjaro|endeavouros) echo "arch" ;;
            alpine) echo "alpine" ;;
            gentoo) echo "gentoo" ;;
            *) echo "unknown" ;;
        esac
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)

case $DISTRO in
    debian)
        SUDO="sudo"
        INSTALL_CMD="apt install -y"
        GIT_PKG="git"
        CMAKE_PKG="cmake"
        NINJA_PKG="ninja-build"
        CLANG_PKG="clang"
        IM_PKG="imagemagick"
        PKGCONFIG_PKG="pkg-config"
        X11_PKG="libx11-dev"
        WAYLAND_PKG="libwayland-dev"
        ;;
    fedora)
        SUDO="sudo"
        INSTALL_CMD="dnf install -y"
        GIT_PKG="git"
        CMAKE_PKG="cmake"
        NINJA_PKG="ninja-build"
        CLANG_PKG="clang"
        IM_PKG="ImageMagick"
        PKGCONFIG_PKG="pkgconf"
        X11_PKG="libX11-devel"
        WAYLAND_PKG="wayland-devel"
        ;;
    arch)
        SUDO="sudo"
        INSTALL_CMD="pacman -S --noconfirm"
        GIT_PKG="git"
        CMAKE_PKG="cmake"
        NINJA_PKG="ninja"
        CLANG_PKG="clang"
        IM_PKG="imagemagick"
        PKGCONFIG_PKG="pkgconf"
        X11_PKG="libx11"
        WAYLAND_PKG="wayland"
        ;;
    alpine)
        SUDO="sudo"
        INSTALL_CMD="apk add"
        GIT_PKG="git"
        CMAKE_PKG="cmake"
        NINJA_PKG="ninja"
        CLANG_PKG="clang"
        IM_PKG="imagemagick"
        PKGCONFIG_PKG="pkgconf"
        X11_PKG="libx11-dev"
        WAYLAND_PKG="wayland-dev"
        ;;
    gentoo)
        SUDO="sudo"
        INSTALL_CMD="emerge"
        GIT_PKG="dev-vcs/git"
        CMAKE_PKG="dev-build/cmake"
        NINJA_PKG="dev-build/ninja"
        CLANG_PKG="sys-devel/clang"
        IM_PKG="media-gfx/imagemagick"
        PKGCONFIG_PKG="dev-util/pkgconf"
        X11_PKG="x11-libs/libX11"
        WAYLAND_PKG="dev-libs/wayland"
        ;;
    *)
        SUDO=""
        INSTALL_CMD=""
        GIT_PKG="git"
        CMAKE_PKG="cmake"
        NINJA_PKG="ninja-build"
        CLANG_PKG="clang"
        IM_PKG="ImageMagick"
        PKGCONFIG_PKG="pkg-config"
        X11_PKG="libX11 development libraries"
        WAYLAND_PKG="wayland development libraries"
        ;;
esac

echo "============================================"
echo "  VEX Engine - Linux Dependency Checker"
echo "============================================"
echo "Detected distro type: $DISTRO"
echo ""

MISSING=()

# Check core tools
if ! command -v git >/dev/null 2>&1; then
    MISSING+=("$GIT_PKG")
    echo "[ERROR] git not found"
else
    echo "[OK] git"
fi

if ! command -v cmake >/dev/null 2>&1; then
    MISSING+=("$CMAKE_PKG")
    echo "[ERROR] cmake not found"
else
    echo "[OK] cmake"
fi

if ! command -v ninja >/dev/null 2>&1; then
    MISSING+=("$NINJA_PKG")
    echo "[ERROR] ninja not found"
else
    echo "[OK] ninja"
fi

if ! command -v clang >/dev/null 2>&1; then
    MISSING+=("$CLANG_PKG")
    echo "[ERROR] clang not found"
else
    echo "[OK] clang"
fi

if ! command -v convert >/dev/null 2>&1; then
    MISSING+=("$IM_PKG (optional)")
    echo "[WARN] ImageMagick missing (checked via 'convert' command)"
else
    echo "[OK] ImageMagick"
fi

echo ""
echo "--- System Libraries (Wayland and X11) ---"

PKGCONFIG_FOUND=0
if command -v pkg-config >/dev/null 2>&1; then
    PKGCONFIG_FOUND=1
else
    MISSING+=("$PKGCONFIG_PKG")
    echo "[ERROR] pkg-config not found - needed for library checks"
fi

if [ $PKGCONFIG_FOUND -eq 1 ]; then
    LIBS_OK=1
    if ! pkg-config --exists x11; then
        MISSING+=("$X11_PKG")
        echo "[ERROR] X11 development libraries not detected"
        LIBS_OK=0
    else
        echo "[OK] X11 development libraries"
    fi
    if ! pkg-config --exists wayland-client; then
        MISSING+=("$WAYLAND_PKG")
        echo "[ERROR] Wayland development libraries not detected"
        LIBS_OK=0
    else
        echo "[OK] Wayland development libraries"
    fi
else
    # If pkg-config missing, assume libs might be missing
    MISSING+=("$X11_PKG")
    MISSING+=("$WAYLAND_PKG")
fi

echo ""
echo "--- Vulkan SDK ---"

VULKAN_FOUND=0
if command -v vulkaninfo >/dev/null 2>&1 || command -v slangc >/dev/null 2>&1; then
    VULKAN_FOUND=1
    echo "[OK] Vulkan SDK"
else
    echo "[ERROR] Vulkan SDK not detected in PATH (vulkaninfo or slangc not found)"
fi

echo ""

if [ ${#MISSING[@]} -gt 0 ]; then
    echo "Missing dependencies (excluding Vulkan):"
    for pkg in "${MISSING[@]}"; do
        echo "- $pkg"
    done
    echo ""

    if [ "$DISTRO" == "unknown" ]; then
        echo "Your distro is not recognized (e.g., Alpine or Gentoo variant?). Please install the missing packages manually using your package manager."
    else
        echo "Suggested install command for missing deps:"
        echo "$SUDO $INSTALL_CMD ${MISSING[*]}"
        echo ""

        read -p "Do you want to install them now? (y/n) " answer
        case "$answer" in
            [Yy]* ) $SUDO $INSTALL_CMD ${MISSING[*]} ;;
            * ) echo "Installation skipped." ;;
        esac
    fi
fi

if [ $VULKAN_FOUND -eq 0 ]; then
    echo ""
    echo "Vulkan SDK is missing. It is recommended to install via official tarball for full support (including slangc if not in repos)."
    read -p "Do you want instructions to install via tarball? (y/n) " tarball_answer
    case "$tarball_answer" in
        [Yy]* )
            echo "1. Download the latest Vulkan SDK tarball from https://vulkan.lunarg.com/sdk/home#linux"
            echo "2. Extract it to a directory, e.g., ~/VulkanSDK/<version>/"
            echo "3. Source the setup-env.sh script: source ~/VulkanSDK/<version>/setup-env.sh"
            echo "   (Add this to your ~/.bashrc or equivalent for permanent setup)"
            echo "4. This will add bin/ to PATH and set other env vars. Ensure validation layers are included."
            echo "5. If slangc is needed separately, it's included in the SDK tarball."
            echo "After setup, re-run this script to verify."
            ;;
        * ) echo "Vulkan installation skipped. Note: Engine may not build without it." ;;
    esac
    FAIL=1
else
    FAIL=0
fi

if [ ${#MISSING[@]} -eq 0 ] && [ $VULKAN_FOUND -eq 1 ]; then
    echo "All dependencies detected."
    exit 0
else
    echo "Dependency check failed."
    exit 1
fi
