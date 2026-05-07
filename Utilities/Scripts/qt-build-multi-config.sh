#!/usr/bin/env bash
# qt-build-multi-config.sh
# Cross-platform helper to build or install Qt mapped to engine configs:
#   Debug  -> Debug
#   Dev    -> RelWithDebInfo (optimized + debug info)
#   Shipping -> Release (strip/split debug symbols on Linux; keep PDBs separate on Windows)
set -euo pipefail
IFS=$'\n\t'

# --- Defaults (override with env vars) ---
QT_VERSION="${QT_VERSION:-6.11.0}"
ENGINE_CONFIG="${ENGINE_CONFIG:-Dev}"   # Debug | Dev | Shipping
PREFIX="${PREFIX:-}"
BUILD_FROM_SOURCE="${BUILD_FROM_SOURCE:-0}"  # Linux only: 0 = use distro packages, 1 = build from source
SRC_ARCHIVE="${SRC_ARCHIVE:-qt-everywhere-src-${QT_VERSION}.zip}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
LOG="${LOG:-$PWD/qt-build.log}"
USE_CCACHE="${USE_CCACHE:-1}"
EXTRA_CONFIGURE_OPTS="${EXTRA_CONFIGURE_OPTS:-}"
# internal
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="${BUILD_ROOT:-$PWD/qt-build}"
SRC_ROOT="${SRC_ROOT:-$PWD/qt-src}"

# Platform detection
is_windows() {
  case "$(uname -s 2>/dev/null || echo Windows)" in
    MINGW*|MSYS*|CYGWIN*|Windows) return 0;;
    *) return 1;;
  esac
}

log() { printf '%s\n' "$*"; }
err() { printf 'ERROR: %s\n' "$*" >&2; }

# Ensure log capture
exec > >(tee -a "$LOG") 2>&1

# --- Helpers ---
command_exists(){ command -v "$1" >/dev/null 2>&1; }

# Map engine config to CMake build type and extra flags
map_config() {
  case "$ENGINE_CONFIG" in
    Debug)
      CMAKE_BUILD_TYPE="Debug"
      CONFIGURE_FLAGS=("-debug")
      ;;
    Dev)
      # Dev -> RelWithDebInfo (optimized with debug info)
      CMAKE_BUILD_TYPE="RelWithDebInfo"
      CONFIGURE_FLAGS=("-release" "-force-debug-info")
      ;;
    Shipping)
      CMAKE_BUILD_TYPE="Release"
      CONFIGURE_FLAGS=("-release")
      ;;
    *)
      err "Unknown ENGINE_CONFIG: $ENGINE_CONFIG. Use Debug, Dev, or Shipping."
      exit 2
      ;;
  esac
}

# --- Linux flow (default uses distro packages unless BUILD_FROM_SOURCE=1) ---
linux_install_packages() {
  if command_exists apt-get; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build python3 perl git \
      libfontconfig1-dev libfreetype-dev libx11-dev libxcb1-dev libxrender-dev \
      libxkbcommon-dev pkg-config ccache || true
  elif command_exists dnf; then
    sudo dnf install -y gcc-c++ cmake ninja-build python3 perl git \
      fontconfig-devel freetype-devel libX11-devel libXkbCommon-devel pkgconfig ccache || true
  else
    log "Unsupported package manager. Please install build deps manually."
  fi
}

linux_fetch_and_extract() {
  mkdir -p "$SRC_ROOT"
  cd "$SRC_ROOT"
  if [ ! -f "$SRC_ARCHIVE" ]; then
    log "Please place Qt source archive at $SRC_ARCHIVE or set SRC_ARCHIVE."
    exit 1
  fi
  # support tar.xz or zip
  case "$SRC_ARCHIVE" in
    *.tar.*|*.tgz)
      tar -xf "$SRC_ARCHIVE"
      ;;
    *.zip)
      unzip -q "$SRC_ARCHIVE"
      ;;
    *)
      err "Unknown archive format: $SRC_ARCHIVE"
      exit 1
      ;;
  esac
  # normalize source dir
  EXTRACTED_DIR=$(tar -tf "$SRC_ARCHIVE" 2>/dev/null | head -1 | cut -f1 -d"/" || true)
  if [ -z "$EXTRACTED_DIR" ]; then
    # fallback: find directory named qt-everywhere*
    EXTRACTED_DIR=$(ls -d qt-everywhere* 2>/dev/null | head -n1 || true)
  fi
  if [ -z "$EXTRACTED_DIR" ]; then
    err "Could not find extracted source directory."
    exit 1
  fi
  mv "$EXTRACTED_DIR" qt-everywhere-src || true
}

# Build on Linux (single-config Ninja recommended)
linux_build() {
  map_config
  linux_install_packages
  if [ "$BUILD_FROM_SOURCE" -ne 1 ]; then
    log "Using system Qt packages (not building from source)."
    log "If you want to build from source set BUILD_FROM_SOURCE=1 and re-run."
    return 0
  fi

  linux_fetch_and_extract
  SRC_DIR="$SRC_ROOT/qt-everywhere-src"
  mkdir -p "$BUILD_ROOT"
  cd "$BUILD_ROOT"

  # For single-config generators (Ninja) create separate build dirs per config
  BUILD_DIR="$BUILD_ROOT/build-${CMAKE_BUILD_TYPE,,}"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  # ccache
  if [ "$USE_CCACHE" -eq 1 ] && command_exists ccache; then
    export CC="ccache gcc"
    export CXX="ccache g++"
    log "Using ccache for gcc/g++"
  fi

  # Run Qt configure (Qt 6 uses configure script that wraps CMake)
  CONFIG_OPTS=(
    -prefix "${PREFIX:-/opt/Qt-${QT_VERSION}}"
    "${CONFIGURE_FLAGS[@]}"
    -nomake examples
    -nomake tests
  )
  # append extras
  if [ -n "$EXTRA_CONFIGURE_OPTS" ]; then
    read -r -a EXTRA <<< "$EXTRA_CONFIGURE_OPTS"
    CONFIG_OPTS+=("${EXTRA[@]}")
  fi

  log "Configuring Qt ($ENGINE_CONFIG -> $CMAKE_BUILD_TYPE) in $BUILD_DIR"
  "$SRC_DIR/configure" "${CONFIG_OPTS[@]}"

  log "Building Qt ($JOBS jobs)"
  cmake --build . --parallel "$JOBS"

  log "Installing to ${PREFIX:-/opt/Qt-${QT_VERSION}}"
  sudo cmake --install .

  if [ "$ENGINE_CONFIG" = "Shipping" ]; then
    log "Splitting debug symbols and stripping binaries for Shipping build"
    # find shared libs and executables under prefix and split debug info
    INSTALL_PREFIX="${PREFIX:-/opt/Qt-${QT_VERSION}}"
    DBG_DIR="$INSTALL_PREFIX/debug-symbols"
    sudo mkdir -p "$DBG_DIR"
    # iterate libs and binaries
    while IFS= read -r -d '' file; do
      sudo objcopy --only-keep-debug "$file" "$DBG_DIR/$(basename "$file").debug" || true
      sudo strip --strip-unneeded "$file" || true
      sudo objcopy --add-gnu-debuglink="$DBG_DIR/$(basename "$file").debug" "$file" || true
    done < <(sudo find "$INSTALL_PREFIX" -type f \( -name "*.so*" -o -perm /111 \) -print0)
    log "Debug symbols saved to $DBG_DIR"
  fi

  log "Linux build complete."
}

# --- Windows flow ---
windows_locate_vcvars() {
  # Try to find vcvarsall.bat via vswhere if available
  if command_exists vswhere; then
    VSWHERE_PATH="$(vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null || true)"
    if [ -n "$VSWHERE_PATH" ]; then
      VCVARS="$VSWHERE_PATH/VC/Auxiliary/Build/vcvarsall.bat"
      if [ -f "$VCVARS" ]; then
        echo "$VCVARS"
        return 0
      fi
    fi
  fi
  # fallback: search common locations
  for base in "/c/Program Files (x86)/Microsoft Visual Studio" "/c/Program Files/Microsoft Visual Studio"; do
    if [ -d "$base" ]; then
      found=$(find "$base" -type f -name vcvarsall.bat 2>/dev/null | head -n1 || true)
      if [ -n "$found" ]; then
        echo "$found"
        return 0
      fi
    fi
  done
  return 1
}

windows_prepare_source() {
  mkdir -p "$SRC_ROOT"
  cd "$SRC_ROOT"
  if [ ! -d "qt-everywhere-src" ]; then
    if [ -f "$SRC_ARCHIVE" ]; then
      unzip -q "$SRC_ARCHIVE"
      mv qt-everywhere-src-* qt-everywhere-src || true
    else
      err "Qt source archive not found at $SRC_ARCHIVE. Please download and set SRC_ARCHIVE."
      exit 1
    fi
  fi
}

windows_build() {
  map_config
  require_cmd() { command -v "$1" >/dev/null 2>&1 || { err "Missing required: $1"; exit 2; }; }

  require_cmd cmake
  require_cmd ninja
  require_cmd python
  require_cmd unzip

  # Ensure MSVC environment
  if ! command_exists cl; then
    VCVARS=$(windows_locate_vcvars || true)
    if [ -z "$VCVARS" ]; then
      err "MSVC compiler not found and vcvarsall.bat not located. Run this script from a 'x64 Native Tools Command Prompt' or set PATH."
      exit 1
    fi
    log "Sourcing vcvarsall.bat via cmd.exe (this will set MSVC env for the session)"
    # Note: we cannot source a .bat into bash; instead we will run configure/build via cmd.exe below.
  fi

  windows_prepare_source
  SRC_DIR="$SRC_ROOT/qt-everywhere-src"
  mkdir -p "$BUILD_ROOT"
  cd "$BUILD_ROOT"

  # Use a multi-config build (Visual Studio or Ninja Multi-Config) so we can build specific config
  BUILD_DIR="$BUILD_ROOT/qt-build"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  # Build steps executed via cmd.exe to ensure MSVC env is active
  # Compose configure.bat command
  WIN_PREFIX="${PREFIX:-C:\\Qt\\${QT_VERSION}}"
  CONFIG_OPTS=(
    -prefix "\"$WIN_PREFIX\""
    "${CONFIGURE_FLAGS[@]}"
    -nomake examples
    -nomake tests
  )
  if [ -n "$EXTRA_CONFIGURE_OPTS" ]; then
    CONFIG_OPTS+=($EXTRA_CONFIGURE_OPTS)
  fi

  # Build commands: run configure.bat then cmake --build with --config
  # Use cmd.exe /c to run batch commands in a single MSVC environment
  CMD_SCRIPT=$(mktemp --suffix=.cmd)
  cat > "$CMD_SCRIPT" <<-CMD
    @echo off
    REM Attempt to call vcvarsall if available
    CALL "${VCVARS:-vcvarsall.bat}" amd64 2>nul || echo "vcvarsall not called; ensure you are in a Developer Prompt"
    cd /d "%~dp0"
    pushd "$SRC_DIR"
    configure.bat ${CONFIG_OPTS[*]}
    popd
    cd /d "%~dp0"
    cmake --build . --config ${CMAKE_BUILD_TYPE} -- /m:${JOBS}
    cmake --install . --config ${CMAKE_BUILD_TYPE} --prefix "$WIN_PREFIX"
    CMD

  log "Running Windows build script via cmd.exe: $CMD_SCRIPT"
  cmd.exe /c "$CMD_SCRIPT" || { err "Windows build failed"; rm -f "$CMD_SCRIPT"; exit 1; }
  rm -f "$CMD_SCRIPT"

  if [ "$ENGINE_CONFIG" = "Shipping" ]; then
    log "Shipping build on Windows: keeping PDBs separate and packaging Release binaries."
    # PDBs are generated alongside binaries; move them to a debug-symbols folder
    WIN_INSTALL="$(cygpath -u "$WIN_PREFIX" 2>/dev/null || echo "$WIN_PREFIX")"
    DBG_DIR="$WIN_INSTALL/debug-symbols"
    mkdir -p "$DBG_DIR"
    # Move PDBs
    find "$WIN_INSTALL" -type f -name "*.pdb" -exec mv {} "$DBG_DIR/" \; || true
    log "PDBs moved to $DBG_DIR. Distribute binaries without PDBs for Shipping."
  fi

  log "Windows build complete. Installed to $WIN_PREFIX"
}

# --- Entrypoint ---
main() {
  log "=== Qt build helper started: $(date) ==="
  map_config

  if is_windows; then
    log "Platform: Windows"
    windows_build
    exit 0
  else
    log "Platform: Linux/Unix"
    linux_build
    exit 0
  fi
}

main "$@"
