#!/usr/bin/env bash
# qt-build-multi-config.sh
# Cross-platform Qt build helper (relative to CWD) that uses Ninja on both Linux and Windows.
# - Uses Ninja as the CMake generator everywhere (requires ninja in PATH).
# - Builds Qt on Windows using MSVC toolchain (vcvars called inside generated .cmd).
# - Downloads Qt source archive automatically (prefers aria2, supports browser mode).
# - Keeps temporary Windows .cmd and captures cmd.exe output when KEEP_CMD=1.
#
# Usage examples:
#   ENGINE_CONFIG=Dev ./qt-build-multi-config.sh
#   ENGINE_CONFIG=Shipping JOBS=8 FAST_DOWNLOAD=browser ./qt-build-multi-config.sh
#   KEEP_CMD=1 ./qt-build-multi-config.sh   # keep the generated .cmd and preserve cmd output
set -euo pipefail
IFS=$'\n\t'

# -------------------------
# User-configurable env vars
# -------------------------
QT_VERSION="${QT_VERSION:-6.11.0}"
ENGINE_CONFIG="${ENGINE_CONFIG:-Dev}"           # Debug | Dev | Shipping
BUILD_FROM_SOURCE="${BUILD_FROM_SOURCE:-1}"     # Linux: 0 = use distro packages, 1 = build from source
SRC_ARCHIVE="${SRC_ARCHIVE:-src/qt-everywhere-src-${QT_VERSION}.zip}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
USE_CCACHE="${USE_CCACHE:-1}"
EXTRA_CONFIGURE_OPTS="${EXTRA_CONFIGURE_OPTS:-}"
FAST_DOWNLOAD="${FAST_DOWNLOAD:-auto}"          # auto | browser
KEEP_CMD="${KEEP_CMD:-0}"                       # 1 to keep the generated .cmd and preserve cmd output

# Force Ninja generator everywhere
CMAKE_GENERATOR="Ninja"
CMAKE_GENERATOR_PLATFORM="${CMAKE_GENERATOR_PLATFORM:-x64}"  # unused for Ninja but kept for compatibility

# -------------------------
# Paths (all relative to CWD)
# -------------------------
ROOT_DIR="$(pwd)"
SRC_ROOT="$ROOT_DIR/src"
BUILD_ROOT="$ROOT_DIR/build"
INSTALL_ROOT="$ROOT_DIR/install"
ARTIFACTS_DIR="$ROOT_DIR/artifacts"
LOG="$ROOT_DIR/qt-build.log"
CMD_LOG="$ROOT_DIR/cmd-run-output.txt"

# capture all output to log
exec > >(tee -a "$LOG") 2>&1

# -------------------------
# Helpers
# -------------------------
log(){ printf '%s\n' "$*"; }
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

# -------------------------
# Download helpers
# -------------------------
qt_download_url_for() {
  local version="$1"
  local tarball="$2"
  local major_minor="${version%.*}"
  printf "https://download.qt.io/official_releases/qt/%s/%s/single/%s" "$major_minor" "$version" "$tarball"
}

download_with_available_tool() {
  local url="$1"
  local out="$2"

  mkdir -p "$(dirname "$out")"

  if [ "${FAST_DOWNLOAD}" = "browser" ]; then
    log "FAST_DOWNLOAD=browser set. Please download the archive manually from:"
    log "$url"
    log "Place the downloaded file at: $out"
    return 2
  fi

  if command_exists aria2c; then
    log "Using aria2c for parallel download -> $out"
    aria2c -x16 -s16 --retry-wait=5 --max-tries=5 --continue=true -o "$(basename "$out")" -d "$(dirname "$out")" "$url"
    return $?
  fi

  if command_exists curl; then
    log "Using curl (resumable) -> $out"
    curl --fail --location --retry 5 --retry-delay 5 --continue-at - -o "$out" "$url"
    return $?
  fi

  if command_exists wget; then
    log "Using wget (resumable) -> $out"
    wget -c --tries=5 --waitretry=5 -O "$out" "$url"
    return $?
  fi

  if command_exists pwsh; then
    log "Using pwsh Invoke-WebRequest -> $out"
    pwsh -NoProfile -Command "try { Invoke-WebRequest -Uri '$url' -OutFile '$out' } catch { exit 1 }"
    return $?
  fi

  if command_exists powershell.exe; then
    log "Using powershell.exe Invoke-WebRequest -> $out"
    powershell.exe -NoProfile -Command "try { Invoke-WebRequest -Uri '$url' -OutFile '$out' -UseBasicParsing } catch { exit 1 }"
    return $?
  fi

  return 1
}

determine_default_archive() {
  if is_windows; then
    echo "src/qt-everywhere-src-${QT_VERSION}.zip"
  else
    echo "src/qt-everywhere-src-${QT_VERSION}.tar.xz"
  fi
}

ensure_source_archive() {
  mkdir -p "$SRC_ROOT"

  if [ -z "${SRC_ARCHIVE:-}" ] || { [ "$SRC_ARCHIVE" = "src/qt-everywhere-src-${QT_VERSION}.zip" ] && ! is_windows; }; then
    SRC_ARCHIVE="$(determine_default_archive)"
  fi

  if [[ "$SRC_ARCHIVE" != /* && ! "$SRC_ARCHIVE" =~ ^[A-Za-z]:\\ ]]; then
    SRC_ARCHIVE="$ROOT_DIR/${SRC_ARCHIVE#./}"
  fi

  if [ -f "$SRC_ARCHIVE" ]; then
    log "Found Qt source archive at $SRC_ARCHIVE"
    return 0
  fi

  local tarball
  if is_windows; then
    tarball="qt-everywhere-src-${QT_VERSION}.zip"
  else
    tarball="qt-everywhere-src-${QT_VERSION}.tar.xz"
  fi

  local url
  url="$(qt_download_url_for "$QT_VERSION" "$tarball")"
  local out="$SRC_ROOT/$tarball"

  log "Qt source archive not found locally. Attempting to download $tarball from official Qt servers."
  log "$url"

  if download_with_available_tool "$url" "$out"; then
    log "Downloaded Qt archive to $out"
    SRC_ARCHIVE="$out"
    return 0
  else
    rc=$?
    if [ "$rc" -eq 2 ]; then
      log "Manual download requested. Re-run after placing the file at: $out"
      exit 0
    fi
    if is_windows; then
      tarball="qt-everywhere-src-${QT_VERSION}.tar.xz"
    else
      tarball="qt-everywhere-src-${QT_VERSION}.zip"
    fi
    url="$(qt_download_url_for "$QT_VERSION" "$tarball")"
    out="$SRC_ROOT/$tarball"
    log "Primary download failed; trying fallback URL:"
    log "$url"
    if download_with_available_tool "$url" "$out"; then
      log "Downloaded Qt archive to $out"
      SRC_ARCHIVE="$out"
      return 0
    fi

    log "Automatic download failed. You can download faster via your browser from:"
    log "$(qt_download_url_for "$QT_VERSION" "qt-everywhere-src-${QT_VERSION}.zip")"
    log "or"
    log "$(qt_download_url_for "$QT_VERSION" "qt-everywhere-src-${QT_VERSION}.tar.xz")"
    log "Place the downloaded file in: $SRC_ROOT and re-run the script, or set SRC_ARCHIVE to the file path."
    err "Failed to obtain Qt source archive automatically."
  fi
}

# -------------------------
# Linux build functions
# -------------------------
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
  ensure_source_archive

  cd "$SRC_ROOT"
  if [ -d "qt-everywhere-src" ]; then
    log "Source already extracted at $SRC_ROOT/qt-everywhere-src"
    return 0
  fi

  case "$SRC_ARCHIVE" in
    *.tar.*|*.tgz|*.tar.xz)
      log "Extracting $SRC_ARCHIVE"
      tar -xf "$SRC_ARCHIVE"
      ;;
    *.zip)
      log "Extracting $SRC_ARCHIVE"
      unzip -q "$SRC_ARCHIVE"
      ;;
    *)
      err "Unknown archive format: $SRC_ARCHIVE"
      ;;
  esac

  EXTRACTED_DIR=$(ls -d qt-everywhere* 2>/dev/null | head -n1 || true)
  if [ -n "$EXTRACTED_DIR" ]; then
    mv -f "$EXTRACTED_DIR" qt-everywhere-src || true
    log "Source prepared at $SRC_ROOT/qt-everywhere-src"
  else
    err "Could not find extracted Qt source directory after unpacking."
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

  log "Configuring CMake (Ninja) for Qt build"
  cmake -G "Ninja" -S "$SRC_DIR" -B "$BUILD_DIR" -D CMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -D CMAKE_INSTALL_PREFIX="$INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE"

  log "Building Qt ($JOBS jobs)"
  cmake --build . --parallel "$JOBS"

  log "Installing to $INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE"
  cmake --install . --prefix "$INSTALL_ROOT/$QT_VERSION/$CMAKE_BUILD_TYPE"

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

# -------------------------
# Windows build functions (Ninja)
# -------------------------
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
  ensure_source_archive

  cd "$SRC_ROOT"
  if [ -d "qt-everywhere-src" ]; then
    log "Source already extracted at $SRC_ROOT/qt-everywhere-src"
    return 0
  fi

  case "$SRC_ARCHIVE" in
    *.zip)
      log "Extracting $SRC_ARCHIVE"
      unzip -q "$SRC_ARCHIVE"
      ;;
    *.tar.*|*.tgz|*.tar.xz)
      log "Extracting $SRC_ARCHIVE"
      tar -xf "$SRC_ARCHIVE"
      ;;
    *)
      err "Unknown archive format: $SRC_ARCHIVE"
      ;;
  esac

  EXTRACTED_DIR=$(ls -d qt-everywhere* 2>/dev/null | head -n1 || true)
  if [ -n "$EXTRACTED_DIR" ]; then
    mv -f "$EXTRACTED_DIR" qt-everywhere-src || true
    log "Source prepared at $SRC_ROOT/qt-everywhere-src"
  else
    err "Could not find extracted Qt source directory after unpacking."
  fi
}

windows_build(){
  map_config
  # require essential tools (ninja is required)
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

  # Windows-style install prefix under the relative install folder (bash form)
  WIN_PREFIX_BASH="${INSTALL_ROOT}/${QT_VERSION}/${CMAKE_BUILD_TYPE}"
  # Convert important paths to Windows form for the .cmd file
  WIN_VCVARS="$(cygpath -w "${VCVARS:-vcvarsall.bat}" 2>/dev/null || echo "${VCVARS:-vcvarsall.bat}")"
  WIN_SRC_DIR="$(cygpath -w "$SRC_DIR" 2>/dev/null || echo "$SRC_DIR")"
  WIN_BUILD_DIR="$(cygpath -w "$BUILD_DIR" 2>/dev/null || echo "$BUILD_DIR")"
  WIN_PREFIX="$(cygpath -w "$WIN_PREFIX_BASH" 2>/dev/null || echo "$WIN_PREFIX_BASH")"

  CONFIG_OPTS=(
    -prefix "\"$WIN_PREFIX\""
    "${CONFIGURE_FLAGS[@]}"
    -nomake examples
    -nomake tests
  )
  if [ -n "$EXTRA_CONFIGURE_OPTS" ]; then
    # split EXTRA_CONFIGURE_OPTS on whitespace safely
    read -r -a EXTRA_ARR <<< "$EXTRA_CONFIGURE_OPTS"
    CONFIG_OPTS+=("${EXTRA_ARR[@]}")
  fi

  # Join CONFIG_OPTS into a single command-line string (no embedded newlines)
  CONFIG_LINE="$(printf '%s ' "${CONFIG_OPTS[@]}")"
  CONFIG_LINE="${CONFIG_LINE%" "}"   # trim trailing space

  # Create a temporary .cmd that calls vcvarsall and runs configure/build/install using Ninja
  CMD_SCRIPT="$(mktemp --suffix=.cmd)"
  cat > "$CMD_SCRIPT" <<EOF
@echo off
REM Ensure MSVC environment
CALL "${WIN_VCVARS}" amd64 2>nul || echo "vcvarsall not called; ensure you are in a Developer Prompt"

REM Ensure build directory exists and switch to it
if not exist "${WIN_BUILD_DIR}" (
  mkdir "${WIN_BUILD_DIR}"
)
pushd "${WIN_BUILD_DIR}"

REM Run configure from source tree (single-line command)
pushd "${WIN_SRC_DIR}"
configure.bat ${CONFIG_LINE}
if ERRORLEVEL 1 (
  echo configure.bat failed with error %ERRORLEVEL%
  popd
  popd
  exit /b %ERRORLEVEL%
)
popd

REM Configure CMake for Ninja in the build dir
cmake -G "Ninja" -S "${WIN_SRC_DIR}" -B "%CD%" -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -D CMAKE_INSTALL_PREFIX="${WIN_PREFIX}"
if ERRORLEVEL 1 (
  echo cmake configuration failed with error %ERRORLEVEL%
  popd
  exit /b %ERRORLEVEL%
)

REM Build with Ninja (parallel)
cmake --build "%CD%" --parallel ${JOBS}
if ERRORLEVEL 1 (
  echo build failed with error %ERRORLEVEL%
  popd
  exit /b %ERRORLEVEL%
)

REM Install
cmake --install "%CD%" --prefix "${WIN_PREFIX}"
if ERRORLEVEL 1 (
  echo install failed with error %ERRORLEVEL%
  popd
  exit /b %ERRORLEVEL%
)

popd
EOF

  log "Running Windows build via cmd.exe (script: $CMD_SCRIPT)"
  # run and capture output to CMD_LOG
  cmd.exe /c "$CMD_SCRIPT" > "$CMD_LOG" 2>&1
  RC=$?
  if [ $RC -ne 0 ]; then
    log "cmd.exe returned non-zero ($RC). See $CMD_LOG for full output."
    if [ "${KEEP_CMD:-0}" -eq 1 ]; then
      log "Keeping temporary script: $CMD_SCRIPT"
    else
      rm -f "$CMD_SCRIPT"
    fi
    err "Windows build failed (see $CMD_LOG)"
  else
    log "Windows build completed successfully. Full output in $CMD_LOG"
    if [ "${KEEP_CMD:-0}" -eq 0 ]; then
      rm -f "$CMD_SCRIPT"
    else
      log "Temporary script retained at $CMD_SCRIPT"
    fi
  fi

  if [ "$ENGINE_CONFIG" = "Shipping" ]; then
    WIN_INSTALL="$(cygpath -u "$WIN_PREFIX" 2>/dev/null || echo "$WIN_PREFIX")"
    DBG_DIR="$WIN_INSTALL/debug-symbols"
    mkdir -p "$DBG_DIR"
    find "$WIN_INSTALL" -type f -name "*.pdb" -exec mv {} "$DBG_DIR/" \; || true
    log "PDBs moved to $DBG_DIR"
  fi

  log "Windows build finished. Installed to $WIN_PREFIX"
}

# -------------------------
# Entrypoint
# -------------------------
main(){
  log "Starting Qt build helper in CWD: $ROOT_DIR"
  map_config
  mkdir -p "$SRC_ROOT" "$BUILD_ROOT" "$INSTALL_ROOT" "$ARTIFACTS_DIR"

  # Ensure ninja is available on both platforms
  if ! command_exists ninja; then
    err "ninja is required but not found in PATH. Install ninja and re-run the script."
  fi

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
