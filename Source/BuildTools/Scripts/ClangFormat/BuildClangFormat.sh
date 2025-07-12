#!/bin/bash

set -e

# Directories in the current working directory
SRC_DIR="./llvm-project"
BUILD_DIR="./llvm-build"
TOOLS_DIR="./tools/precompiled"
INSTALL_DIR="$HOME/.local/llvm"  # Optional for add_to_path

# Clean up previous artifacts
cleanup() {
  echo "Cleaning previous build directories..."
  rm -rf "$SRC_DIR" "$BUILD_DIR"
}

# Install dependencies per distro
install_deps() {
  echo "Installing dependencies for $ID..."
  source /etc/os-release
  case "$ID" in
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
      echo "Unsupported distro: $ID"
      exit 1
      ;;
  esac
}

# Clone and build clang-format
build_clang_format() {
  echo "Cloning LLVM source..."
  git clone --depth=1 https://github.com/llvm/llvm-project.git "$SRC_DIR"

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

# Copy the binary out for packaging
package_binary() {
  echo "Copying clang-format to $TOOLS_DIR..."
  mkdir -p "$TOOLS_DIR"
  cp "$BUILD_DIR/bin/clang-format" "$TOOLS_DIR/"
}

# Optionally add to PATH
add_to_path() {
  echo "Adding clang-format to PATH at $INSTALL_DIR/bin..."
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

# To enable direct CLI usage, uncomment the following line:
# add_to_path

echo "clang-format binary is now in: $TOOLS_DIR/clang-format"
