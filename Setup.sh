#!/bin/bash
set -euo pipefail

echo "=== XII Linux Setup Script (Dependencies + Toolchains) ==="

# ----------------------------------------
# Detect distribution
# ----------------------------------------
if [[ ! -f /etc/os-release ]]; then
  echo "Cannot detect distribution (missing /etc/os-release)"
  exit 1
fi

. /etc/os-release
DISTRO=$ID
VER=$VERSION_ID

echo "Detected: $DISTRO $VER"

# ----------------------------------------
# Debian-based package lists
# ----------------------------------------
DEB_BASE_PKGS=(
  cmake
  ninja-build
  git
  uuid-dev
  libfreetype-dev
  libtinfo5
  mold
  clang
  clang-tools
  libstdc++-dev
)

DEB_WAYLAND_PKGS=(
  libwayland-dev
  libwayland-egl1
  libwayland-cursor0
)

DEB_QT_PKGS=(
  qt6-base-dev
  qt6-base-private-dev
  libqt6svg6-dev
  qt6-wayland
)

# ----------------------------------------
# Fedora package lists
# ----------------------------------------
FEDORA_BASE_PKGS=(
  gcc
  gcc-c++
  clang
  clang-tools-extra
  libstdc++-devel
  egl-wayland
  libuuid-devel
  freetype-devel
  ncurses-compat-libs
  mold
)

FEDORA_QT_PKGS=(
  qt6-qtbase-devel
  qt6-qtsvg-devel
  qt6-qtbase-private-devel
  qt6-qtwayland-devel
)

# ----------------------------------------
# Install dependencies
# ----------------------------------------
case "$DISTRO" in
  ubuntu|linuxmint|kali)
    echo "Installing Debian-based dependencies..."
    sudo apt update
    sudo apt install -y \
      "${DEB_BASE_PKGS[@]}" \
      "${DEB_WAYLAND_PKGS[@]}" \
      "${DEB_QT_PKGS[@]}"

    echo "Installing Vulkan packages..."
    sudo apt install -y libvulkan-dev vulkan-tools
    ;;
  fedora)
    echo "Installing Fedora dependencies..."
    sudo dnf install -y \
      "${FEDORA_BASE_PKGS[@]}" \
      "${FEDORA_QT_PKGS[@]}"

    echo "Installing Vulkan packages..."
    sudo dnf install -y vulkan-loader-devel vulkan-tools
    ;;
  *)
    echo "Unsupported distribution: $DISTRO"
    exit 1
    ;;
esac

# ----------------------------------------
# Compiler validation
# ----------------------------------------
echo "Checking compiler versions..."

GCC_VER=$(gcc -dumpversion | cut -d. -f1)
CLANG_VER=$(clang --version 2>/dev/null | head -n1 | grep -o "[0-9]\+" | head -n1 || echo 0)

echo "GCC version: $GCC_VER"
echo "Clang version: $CLANG_VER"

if [[ "$GCC_VER" -lt 12 ]]; then
  echo "Warning: GCC >= 12 recommended."
fi

if [[ "$CLANG_VER" -lt 15 ]]; then
  echo "Warning: Clang >= 15 recommended."
fi

echo ""
echo "=== Setup complete ==="
echo "You can now run your Linux generate script."
