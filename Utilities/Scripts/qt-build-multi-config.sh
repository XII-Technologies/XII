#!/usr/bin/env bash
# qt-build-multi-config.sh
# Cross-platform Qt build helper that keeps all files relative to the current working directory.
# Usage examples:
#   # Bash on Windows (started from VS Developer Prompt) or Linux:
#   ./qt-build-multi-config.sh
#   ENGINE_CONFIG=Shipping JOBS=8 ./qt-build-multi-config.sh
set -euo pipefail
IFS=$'\n\t'

# --- Configurable env vars (override before running) ---
QT_VERSION="${QT_VERSION:-6.11.0}"
ENGINE_CONFIG="${ENGINE_CONFIG:-Dev}"   # Debug | Dev | Shipping
BUILD_FROM_SOURCE="${BUILD_FROM_SOURCE:-1}"  # Linux: 0 = use distro packages, 1 = build from source
SRC_ARCHIVE="${SRC_ARCHIVE:-src/qt-everywhere-src-${QT_VERSION}.zip}"  # relative to CWD by default
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
USE_CCACHE="${USE_CCACHE:-1}"
EXTRA_CONFIGURE_OPTS="${EXTRA_CONFIGURE_OPTS:-}"

# --- Relative paths ---
ROOT_DIR="$(pwd)"
SRC_ROOT="$ROOT_DIR/src"
BUILD_ROOT="$ROOT_DIR/build"
INSTALL_ROOT="$ROOT_DIR/install"
ARTIFACTS_DIR="$ROOT_DIR/artifacts"
LOG="$ROOT_DIR/qt-build.log"

exec > >(tee -a "$LOG") 2>&1

# --- Helpers ---
log(){ printf '%s\n' "$*"; }
err(){ printf 'ERROR: %s\n' "$*' >&2"; exit 1; }  # note: single-quote in err fixed below

# Fix err function (corrected)
err(){ printf 'ERROR: %s\n' "$*" >&2; exit 1; }

command_exists(){ command -v "$1" >/dev/null 2>&1; }

map_config(){
  case "$ENGINE_CONFIG" in
    Debug)
      CMAKE_BUILD_TYPE="Debug"
      CONFIGURE_FLAGS=("-debug")
      ;;
    Dev)
      CMAKE_BUILD_TYPE="RelWithDebInfo"
      CONFIGURE_FLAGS=("-release" "-force-debug-info")
      ;;
    Shipping)
      CMAKE_BUILD_TYPE="Release"
      CONFIGURE_FLAGS=("-release")
      ;;
    *)
      err "Unknown ENGINE_CONFIG: $ENGINE_CONFIG. Use Debug, Dev, or Shipping."
      ;;
  esac
}

is_windows(){
  case "$(uname -s 2>/dev/null || echo Windows)" in
    MINGW*|MSYS*|CYGWIN*|Windows) return 0;;
    *) return 1;;
  esac
}

# --- Linux functions ---
linux_install_packages(){
  if command_exists apt-get; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build python3 perl git \
      libfontconfig1-dev libfreetype-dev libx11-dev libxcb1-dev libxrender-dev \
      libxkbcommon-dev pkg-config ccache || true
  elif command_exists dnf; then
    sudo dnf install -y gcc-c++ cmake ninja-build python3 perl git \
      fontconfig-devel freetype-devel libX11-devel libXkbCommon-devel pkgconfig ccache || true
  else
    log "Unsupported package manager; install deps manually."
  fi
}

linux_prepare_source(){
  mkdir -p "$SRC_ROOT"
  cd "$SRC_ROOT"
  if [ ! -d "qt-everywhere-src" ]; then
    if [ -f "$ROOT_DIR/$SRC_ARCHIVE" ]; then
      case "$SRC_ARCHIVE" in
        *.tar.*|*.tgz) tar -xf "$ROOT_DIR/$SRC_ARCHIVE" ;;
        *.zip) unzip -q "$ROOT_DIR/$SRC_ARCHIVE" ;;
        *) err "Unknown archive format: $SRC_ARCHIVE" ;;
      esac
      # normalize
      EXTRACTED_DIR=$(ls -d qt-everywhere* 2>/dev/null | head -n1 || true)
      [ -n "$EXTRACTED_DIR" ] && mv "$EXTRACTED_DIR" qt-everywhere-src || true
    else
      err "Source archive not found at $ROOT_DIR/$SRC_ARCHIVE"
    fi
  fi
}

linux_build(){
  map_config
  linux_install_packages
  if [ "$BUILD_FROM_SOURCE" -ne 1 ]; then
    log "Using system Qt packages (not building from source)."
    return 0
  fi

  linux_prepare_source
  SRC_DIR="$SRC_ROOT/qt-everywhere-src"
  BUILD_DIR="$BUILD_ROOT/build-${CMAKE_BUILD_TYPE,,}"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  if [ "$USE_CCACHE" -eq 1 ] && command_exists ccache; then
    export CC="ccache gcc"
    export CXX="ccache g++"
    log "Using ccache"
  fi

  CONFIG_OPTS=(
    -prefix "$INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE"
    "${CONFIGURE_FLAGS[@]}"
    -nomake examples
    -nomake tests
  )
  if [ -n "$EXTRA_CONFIGURE_OPTS" ]; then
    read -r -a EXTRA <<< "$EXTRA_CONFIGURE_OPTS"
    CONFIG_OPTS+=("${EXTRA[@]}")
  fi

  log "Configuring Qt in $BUILD_DIR"
  "$SRC_DIR/configure" "${CONFIG_OPTS[@]}"

  log "Building Qt ($JOBS jobs)"
  cmake --build . --parallel "$JOBS"

  log "Installing to $INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE"
  sudo cmake --install .

  if [ "$ENGINE_CONFIG" = "Shipping" ]; then
    log "Splitting debug symbols and stripping binaries for Shipping build"
    DBG_DIR="$INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE/debug-symbols"
    sudo mkdir -p "$DBG_DIR"
    while IFS= read -r -d '' file; do
      sudo objcopy --only-keep-debug "$file" "$DBG_DIR/$(basename "$file").debug" || true
      sudo strip --strip-unneeded "$file" || true
      sudo objcopy --add-gnu-debuglink="$DBG_DIR/$(basename "$file").debug" "$file" || true
    done < <(sudo find "$INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE" -type f \( -name "*.so*" -o -perm /111 \) -print0)
    log "Debug symbols saved to $DBG_DIR"
  fi

  log "Linux build finished."
}

# --- Windows functions ---
windows_locate_vcvars(){
  if command_exists vswhere; then
    VSROOT=$(vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>/dev/null || true)
    [ -n "$VSROOT" ] && echo "$VSROOT/VC/Auxiliary/Build/vcvarsall.bat" && return 0
  fi
  for base in "/c/Program Files (x86)/Microsoft Visual Studio" "/c/Program Files/Microsoft Visual Studio"; do
    if [ -d "$base" ]; then
      found=$(find "$base" -type f -name vcvarsall.bat 2>/dev/null | head -n1 || true)
      [ -n "$found" ] && echo "$found" && return 0
    fi
  done
  return 1
}

windows_prepare_source(){
  mkdir -p "$SRC_ROOT"
  cd "$SRC_ROOT"
  if [ ! -d "qt-everywhere-src" ]; then
    if [ -f "$ROOT_DIR/$SRC_ARCHIVE" ]; then
      unzip -q "$ROOT_DIR/$SRC_ARCHIVE"
      mv qt-everywhere-src-* qt-everywhere-src || true
    else
      err "Qt source archive not found at $ROOT_DIR/$SRC_ARCHIVE"
    fi
  fi
}

windows_build(){
  map_config
  for tool in cmake ninja python unzip; do
    command_exists "$tool" || err "Missing required tool: $tool (install and add to PATH)"
  done

  windows_prepare_source
  SRC_DIR="$SRC_ROOT/qt-everywhere-src"
  BUILD_DIR="$BUILD_ROOT/qt-build"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  cd "$BUILD_DIR"

  VCVARS="$(windows_locate_vcvars || true)"
  if [ -z "$VCVARS" ] && ! command_exists cl; then
    err "MSVC not found. Start Git Bash from a Visual Studio Developer Prompt or ensure vcvarsall.bat is available."
  fi

  # Windows-style install prefix under the relative install folder
  WIN_PREFIX="${INSTALL_ROOT//\//\\}\\$QT_VERSION\\$CMAKE_BUILD_TYPE"

  CONFIG_OPTS=(
    -prefix "\"$WIN_PREFIX\""
    "${CONFIGURE_FLAGS[@]}"
    -nomake examples
    -nomake tests
  )
  if [ -n "$EXTRA_CONFIGURE_OPTS" ]; then
    CONFIG_OPTS+=($EXTRA_CONFIGURE_OPTS)
  fi

  # Create a temporary .cmd that calls vcvarsall and runs configure/build/install
  CMD_SCRIPT="$(mktemp --suffix=.cmd)"
  cat > "$CMD_SCRIPT" <<EOF
@echo off
CALL "${VCVARS:-vcvarsall.bat}" amd64 2>nul || echo "vcvarsall not called; ensure you are in a Developer Prompt"
pushd "%~dp0"
pushd "$SRC_DIR"
configure.bat ${CONFIG_OPTS[*]}
popd
cmake --build . --config ${CMAKE_BUILD_TYPE} -- /m:${JOBS}
cmake --install . --config ${CMAKE_BUILD_TYPE} --prefix "$WIN_PREFIX"
popd
EOF

  log "Running Windows build via cmd.exe (script: $CMD_SCRIPT)"
  cmd.exe /c "$CMD_SCRIPT" || { rm -f "$CMD_SCRIPT"; err "Windows build failed"; }
  rm -f "$CMD_SCRIPT"

  if [ "$ENGINE_CONFIG" = "Shipping" ]; then
    WIN_INSTALL="$(cygpath -u "$WIN_PREFIX" 2>/dev/null || echo "$WIN_PREFIX")"
    DBG_DIR="$WIN_INSTALL/debug-symbols"
    mkdir -p "$DBG_DIR"
    find "$WIN_INSTALL" -type f -name "*.pdb" -exec mv {} "$DBG_DIR/" \; || true
    log "PDBs moved to $DBG_DIR"
  fi

  log "Windows build finished. Installed to $WIN_PREFIX"
}

# --- Entrypoint ---
main(){
  log "Starting Qt build helper in CWD: $ROOT_DIR"
  map_config
  mkdir -p "$SRC_ROOT" "$BUILD_ROOT" "$INSTALL_ROOT" "$ARTIFACTS_DIR"

  if is_windows; then
    log "Platform: Windows (bash). All paths are relative to $ROOT_DIR"
    windows_build
  else
    log "Platform: Linux/Unix. All paths are relative to $ROOT_DIR"
    linux_build
  fi

  log "Done. Install trees under $INSTALL_ROOT"
}

main "$@"
