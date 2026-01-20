#!/bin/bash -e

# -----------------------------
# Parse arguments
# -----------------------------
opts=$(getopt \
  --longoptions help,clang,setup,no-cmake,no-unitybuild,build-type: \
  --name "$(basename "$0")" \
  --options "" \
  -- "$@"
)

eval set -- "$opts"

RunCMake=true
BuildType="Dev"
NoUnityBuild=""
UseClang=false
Setup=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      echo "Usage: $(basename $0) [--setup] [--clang] [--no-cmake] [--build-type Debug|Dev|Shipping] [--no-unitybuild]"
      exit 0
      ;;
    --clang)        UseClang=true; shift ;;
    --setup)        Setup=true; shift ;;
    --no-cmake)     RunCMake=false; shift ;;
    --no-unitybuild) NoUnityBuild="-DXII_ENABLE_FOLDER_UNITY_FILES=OFF"; shift ;;
    --build-type)   BuildType=$2; shift 2 ;;
    *)              break ;;
  esac
done

if [[ "$BuildType" != "Debug" && "$BuildType" != "Dev" && "$BuildType" != "Shipping" ]]; then
  >&2 echo "Invalid build-type: '$BuildType'. Supported: Debug, Dev, Shipping."
  exit 1
fi

# -----------------------------
# Detect distribution
# -----------------------------
if [ ! -f "/etc/os-release" ]; then
  >&2 echo "/etc/os-release missing. Cannot detect distribution."
  exit 1
fi

. /etc/os-release   # loads ID, VERSION_ID

Distribution=$ID
Version=$VERSION_ID

# -----------------------------
# Version comparison helpers
# -----------------------------
verlte() { [ "$1" = "$(echo -e "$1\n$2" | sort -V | head -n1)" ]; }
verlt()  { [ "$1" = "$2" ] && return 1 || verlte "$1" "$2"; }

# -----------------------------
# Package selection
# -----------------------------
packages=()

case "$Distribution" in
  ubuntu)
    if [[ "$Version" == "22.04" ]]; then
      packages=(cmake build-essential ninja-build libwayland-dev libwayland-egl1 libwayland-cursor0 uuid-dev mold libfreetype-dev libtinfo5)
    fi
    ;;
  linuxmint)
    if [[ "$Version" == "21" ]]; then
      packages=(cmake build-essential ninja-build libwayland-dev libwayland-egl1 libwayland-cursor0 uuid-dev mold libfreetype-dev libtinfo5)
    fi
    ;;
  kali)
    if [[ "$Version" =~ ^2023 ]]; then
      packages=(cmake build-essential ninja-build libwayland-dev libwayland-egl1 libwayland-cursor0 uuid-dev mold libfreetype-dev libtinfo5)
    fi
    ;;
  fedora)
    if [[ "$Version" -ge 38 ]]; then
      packages=(cmake gcc gcc-c++ ninja-build egl-wayland libuuid-devel mold freetype-devel ncurses-compat-libs)
    fi
    ;;
esac

if [[ ${#packages[@]} -eq 0 ]]; then
  >&2 echo "Unsupported distribution/version: $Distribution $Version"
  >&2 echo "Supported:"
  >&2 echo "  * Ubuntu 22.04"
  >&2 echo "  * Linux Mint 21"
  >&2 echo "  * Kali Rolling 2023"
  >&2 echo "  * Fedora 38+"
  exit 1
fi

# -----------------------------
# Compiler selection
# -----------------------------
if $UseClang; then
  if [[ "$Distribution" == "fedora" ]]; then
    packages+=(clang libstdc++-devel)
  else
    packages+=(clang libstdc++-dev)
  fi
  c_compiler=clang
  cxx_compiler=clang++
else
  if [[ "$Distribution" == "fedora" ]]; then
    packages+=(gcc gcc-c++)
  else
    packages+=(gcc g++)
  fi
  c_compiler=gcc
  cxx_compiler=g++
fi

# -----------------------------
# Setup phase (install packages)
# -----------------------------
if $Setup; then
  if [[ "$Distribution" == "fedora" ]]; then
    qtVer=$(dnf info qt6-qtbase-devel 2>/dev/null | grep -o "6\.[0-9]*\.[0-9]")
    echo "Detected Qt version: $qtVer"
    if verlt "$qtVer" "6.3.0"; then
      >&2 echo -e "\033[0;33mQt >= 6.3.0 not available in Fedora repos. Install manually."
    else
      packages+=(qt6-qtbase-devel qt6-qtsvg-devel qt6-qtbase-private-devel qt6-qtwayland-devel)
    fi
    git submodule update --init
    echo "Installing packages via dnf: ${packages[*]}"
    sudo dnf install -y "${packages[@]}"
  else
    qtVer=$(apt list qt6-base-dev 2>/dev/null | grep -o "6\.[0-9]*\.[0-9]")
    echo "Detected Qt version: $qtVer"
    if verlt "$qtVer" "6.3.0"; then
      >&2 echo -e "\033[0;33mQt >= 6.3.0 not available in apt repos. Install manually."
    else
      packages+=(qt6-base-dev libqt6svg6-dev qt6-base-private-dev qt6-wayland)
    fi
    git submodule update --init
    echo "Installing packages via apt: ${packages[*]}"
    sudo apt install -y "${packages[@]}"
  fi
fi

# -----------------------------
# Compiler version check
# -----------------------------
if $UseClang; then
  clangVer=$(clang --version 2>/dev/null | head -n1 | grep -o "[0-9]\+" | head -n1)
  if [[ -z "$clangVer" || "$clangVer" -lt 18 ]]; then
    >&2 echo "Clang >= 18 required. Found: $clangVer"
    exit 1
  fi
else
  gccVer=$(gcc -dumpversion | cut -d. -f1)
  if [[ -z "$gccVer" || "$gccVer" -lt 14 ]]; then
    >&2 echo "GCC >= 14 required. Found: $gccVer"
    exit 1
  fi
fi

# -----------------------------
# CMake phase
# -----------------------------
CompilerShort=$($UseClang && echo "clang" || echo "gcc")

if $RunCMake; then
  BuildDir="build-${BuildType}-${CompilerShort}"
  cmake -B "$BuildDir" -S . -G Ninja \
    -DCMAKE_CXX_COMPILER="$cxx_compiler" \
    -DCMAKE_C_COMPILER="$c_compiler" \
    -DCMAKE_BUILD_TYPE="$BuildType" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $NoUnityBuild \
    -DXII_BUILD_VULKAN=ON && \
  echo -e "\nRun 'ninja -C ${BuildDir}' to build"
fi
