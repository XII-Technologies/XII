
#!/usr/bin/env bash

# Copyright (c) XII Technologies. All Rights Reserved.
# XII Engine - Linux Development Environment Setup

set -Eeuo pipefail

readonly MIN_GCC_VERSION=12
readonly MIN_CLANG_VERSION=15

log()
{
  printf '[XII Setup] %s\n' "$*"
}

fail()
{
  printf '[XII Setup] ERROR: %s\n' "$*" >&2
  exit 1
}

# ---------------------------------------------------------------------------
# Distribution Detection
# ---------------------------------------------------------------------------

detect_distribution()
{
  [[ -r /etc/os-release ]] ||
    fail "Cannot detect Linux distribution."

  # shellcheck disable=SC1091
  source /etc/os-release

  readonly DISTRO_ID="${ID:-unknown}"
  readonly DISTRO_VERSION="${VERSION_ID:-unknown}"

  case "$DISTRO_ID" in
    fedora)
      PACKAGE_FAMILY="fedora"
      ;;
    debian|ubuntu|linuxmint|kali|pop|elementary)
      PACKAGE_FAMILY="debian"
      ;;
    *)
      case " ${ID_LIKE:-} " in
        *" fedora "*|*" rhel "*)
          PACKAGE_FAMILY="fedora"
          ;;
        *" debian "*|*" ubuntu "*)
          PACKAGE_FAMILY="debian"
          ;;
        *)
          fail "Unsupported Linux distribution: $DISTRO_ID"
          ;;
      esac
      ;;
  esac

  log "Distribution: ${PRETTY_NAME:-$DISTRO_ID}"
  log "Package family: $PACKAGE_FAMILY"
}

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

DEBIAN_PACKAGES=(
  # Build toolchain
  build-essential
  gcc
  g++
  clang
  clang-tools
  cmake
  ninja-build
  make
  mold
  git
  pkg-config

  # Networking dependency builds (OpenSSL / MsQuic)
  python3
  perl

  # Foundation
  uuid-dev
  libfreetype-dev
  libtinfo6

  # Qt 6
  qt6-base-dev
  qt6-base-private-dev
  libqt6svg6-dev
  qt6-wayland

  # Wayland
  libwayland-dev
  libwayland-egl1
  libwayland-cursor0

  # Vulkan
  libvulkan-dev
  vulkan-tools
  vulkan-validationlayers-dev
)

FEDORA_PACKAGES=(
  # Build toolchain
  gcc
  gcc-c++
  clang
  clang-tools-extra
  cmake
  ninja-build
  make
  mold
  git
  pkgconf-pkg-config

  # Networking dependency builds (OpenSSL / MsQuic)
  python3
  perl

  # Foundation
  libstdc++-devel
  libuuid-devel
  freetype-devel
  ncurses-compat-libs

  # Qt 6
  qt6-qtbase-devel
  qt6-qtbase-private-devel
  qt6-qtsvg-devel
  qt6-qtwayland-devel

  # Wayland
  wayland-devel
  egl-wayland

  # Vulkan
  vulkan-loader-devel
  vulkan-validation-layers
  vulkan-tools
)

# ---------------------------------------------------------------------------
# Dependency Installation
# ---------------------------------------------------------------------------

install_dependencies()
{
  local -a privilege=()

  if (( EUID != 0 )); then
    command -v sudo >/dev/null 2>&1 ||
      fail "sudo is required to install system packages."

    privilege=(sudo)
  fi

  case "$PACKAGE_FAMILY" in
    debian)
      command -v apt-get >/dev/null 2>&1 ||
        fail "APT is not available."

      log "Updating APT package metadata..."
      "${privilege[@]}" apt-get update

      log "Installing development dependencies..."
      "${privilege[@]}" apt-get install -y \
        "${DEBIAN_PACKAGES[@]}"
      ;;

    fedora)
      command -v dnf >/dev/null 2>&1 ||
        fail "DNF is not available."

      log "Installing development dependencies..."
      "${privilege[@]}" dnf install -y \
        "${FEDORA_PACKAGES[@]}"
      ;;
  esac
}

# ---------------------------------------------------------------------------
# Environment Validation
# ---------------------------------------------------------------------------

check_command()
{
  local executable="$1"

  command -v "$executable" >/dev/null 2>&1 ||
    fail "Required executable not found: $executable"
}

check_compiler()
{
  local compiler="$1"
  local minimum="$2"
  local version=""
  local major=""

  check_command "$compiler"

  if [[ "$compiler" == "gcc" ]]; then
    version="$(gcc -dumpfullversion -dumpversion)"
    major="${version%%.*}"
  else
    version="$(clang --version | head -n 1)"

    if [[ "$version" =~ version[[:space:]]+([0-9]+) ]]; then
      major="${BASH_REMATCH[1]}"
    else
      fail "Cannot determine Clang version: $version"
    fi
  fi

  [[ "$major" =~ ^[0-9]+$ ]] ||
    fail "Invalid $compiler version: $version"

  log "$compiler: $version"

  if (( 10#$major < minimum )); then
    log "WARNING: $compiler $minimum or newer is recommended."
  fi
}

validate_environment()
{
  log "Validating development environment..."

  local executable
  for executable in \
    gcc g++ clang clang++ cmake ninja git python3 perl make
  do
    check_command "$executable"
  done

  check_compiler gcc "$MIN_GCC_VERSION"
  check_compiler clang "$MIN_CLANG_VERSION"

  # Required by OpenSSL's upstream Configure script.
  if ! perl -MFindBin -MIPC::Cmd -e 'exit 0'; then
    fail "Missing Perl modules required by OpenSSL (FindBin / IPC::Cmd)."
  fi

  log "OpenSSL Perl dependencies: OK"
  log "CMake: $(cmake --version | head -n 1)"
  log "Ninja: $(ninja --version)"
  log "Python: $(python3 --version)"
  log "Perl: $(perl -e 'printf "%vd", $^V')"

  log "Environment validation passed."
}

# ---------------------------------------------------------------------------
# Entry Point
# ---------------------------------------------------------------------------

main()
{
  log "========================================"
  log "XII Engine - Linux Development Setup"
  log "========================================"

  detect_distribution
  install_dependencies
  validate_environment

  log "========================================"
  log "Setup completed successfully."
  log "Run the XII Linux generation script next."
  log "========================================"
}

main "$@"
