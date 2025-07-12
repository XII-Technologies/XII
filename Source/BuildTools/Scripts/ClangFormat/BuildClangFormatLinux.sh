#!/bin/bash

set -e

# Detect Linux distribution
if [ -f /etc/os-release ]; then
  . /etc/os-release
  DISTRO=$ID
else
  echo "Unable to detect Linux distribution."
  exit 1
fi

# Define paths
SRC_DIR="$HOME/src/llvm-project"
BUILD_DIR="$HOME/build/llvm"
TOOLS_DIR="$HOME/tools/precompiled"  # Customize this if needed
INSTALL_DIR="$HOME/.local/llvm"      # Used only for optional PATH inclusion

# Clean up previous artifacts
cleanup() {
  echo "Cleaning previous build directories..."
  rm -rf "$SRC_DIR" "$BUILD_DIR"
}

# Install required packages based on distro
install_deps() {
  echo "Installing dependencies for $DISTRO..."
  case "$DISTRO" in
    fedora)
      sudo dnf install -y git cmake ninja-build gcc-c++ python3 ncurses-devel zlib-devel
      ;;
    arch|manjaro)
      sudo pacman -Syu --noconfirm git cmake ninja gcc python ncurses zlib
      ;;
    ubuntu|debian)
      sudo apt update
      sudo apt install -y git cmake ninja-build build-essential python3 libncurses5-dev zlib1g-dev
      ;;
    *)
      echo "Unsupported distro: $DISTRO"
      exit 1
      ;;
  esac
}

# Clone and build clang-format
build_clang_format() {
  echo "Cloning LLVM source..."
  git clone https://github.com/llvm/llvm-project.git "$SRC_DIR"

  echo "Configuring build..."
  cmake -G Ninja \
    -S "$SRC_DIR/llvm" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DLLVM_ENABLE_ASSERTIONS=ON

  echo "Building clang-format..."
  ninja -C "$BUILD_DIR" clang-format
}

# Package built binary externally
package_binary() {
  echo "Copying clang-format to external tools directory..."
  mkdir -p "$TOOLS_DIR"
  cp "$BUILD_DIR/bin/clang-format" "$TOOLS_DIR/"
}

# Optionally add to shell PATH
add_to_path() {
  echo "Updating PATH variable with optional install location..."
  mkdir -p "$INSTALL_DIR/bin"
  cp "$BUILD_DIR/bin/clang-format" "$INSTALL_DIR/bin/"
  if ! grep -q "$INSTALL_DIR/bin" "$HOME/.bashrc"; then
    echo "export PATH=\"$INSTALL_DIR/bin:\$PATH\"" >> "$HOME/.bashrc"
    source "$HOME/.bashrc"
  fi
}

# Run core steps
cleanup
install_deps
build_clang_format
package_binary

# Optional: uncomment to make clang-format easily available in shell
# add_to_path

echo "clang-format binary is packaged in: $TOOLS_DIR"
