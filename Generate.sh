#!/bin/bash
set -euo pipefail

# ----------------------------------------
# Argument parsing
# ----------------------------------------
print_help() {
  echo "Usage: $(basename "$0") [options]"
  echo ""
  echo "Options:"
  echo "  --clang                 Use Clang instead of GCC"
  echo "  --no-unitybuild         Disable unity builds"
  echo "  --no-submodule-update   Skip git submodule update"
  echo "  --solution-name NAME    Set custom solution name"
  echo "  --workspace DIR         Override workspace directory"
  echo "  --vulkan=[on|off]       Enable/disable Vulkan"
  echo "  --d3d12=[on|off]        Enable/disable D3D12 (ignored on Linux)"
  echo "  --build-type TYPE       Debug | Dev | Shipping"
  echo ""
  exit 0
}

UseClang=true
NoUnityBuild=false
NoSubmoduleUpdate=false
SolutionName=""
WorkspaceDirectory=""
VulkanSupport="on"
D3D12Support="off"
BuildType="Dev"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help) print_help ;;
    --clang) UseClang=true ;;
    --no-unitybuild) NoUnityBuild=true ;;
    --no-submodule-update) NoSubmoduleUpdate=true ;;
    --solution-name) SolutionName="$2"; shift ;;
    --workspace) WorkspaceDirectory="$2"; shift ;;
    --vulkan=*) VulkanSupport="${1#*=}" ;;
    --d3d12=*) D3D12Support="${1#*=}" ;; # ignored but accepted
    --build-type) BuildType="$2"; shift ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
  shift
done

if [[ "$BuildType" != "Debug" && "$BuildType" != "Dev" && "$BuildType" != "Shipping" ]]; then
  echo "Invalid build-type: $BuildType"
  exit 1
fi

# ----------------------------------------
# Submodule update logic
# ----------------------------------------
if ! $NoSubmoduleUpdate; then
  CURRENT_COMMIT=$(git log -n 1 --format=%H)
  LAST_UPDATE_FILE="Data/Content/AssetCache/LastSubmoduleUpdate.txt"

  echo "Current commit: $CURRENT_COMMIT"

  UPDATE_SUBMODULES=true
  if [[ -f "$LAST_UPDATE_FILE" ]]; then
    LAST_COMMIT=$(cat "$LAST_UPDATE_FILE")
    if [[ "$LAST_COMMIT" == "$CURRENT_COMMIT" ]]; then
      echo "Submodules already up-to-date."
      UPDATE_SUBMODULES=false
    else
      echo "Submodules were last updated at commit: $LAST_COMMIT"
    fi
  fi

  if $UPDATE_SUBMODULES; then
    echo "Updating submodules..."
    git submodule update --init
    echo "$CURRENT_COMMIT" > "$LAST_UPDATE_FILE"
  fi
fi

# ----------------------------------------
# Compiler selection
# ----------------------------------------
if $UseClang; then
  C_COMPILER="clang"
  CXX_COMPILER="clang++"
  CompilerShort="clang"
else
  C_COMPILER="gcc"
  CXX_COMPILER="g++"
  CompilerShort="gcc"
fi

# ----------------------------------------
# Workspace directory
# ----------------------------------------
if [[ -z "$WorkspaceDirectory" ]]; then
  WorkspaceDirectory="linux-${BuildType}-${CompilerShort}"
fi

mkdir -p "Workspace/$WorkspaceDirectory"

# ----------------------------------------
# CMake argument construction
# ----------------------------------------
CMAKE_ARGS=(
  -S .
  -B "Workspace/$WorkspaceDirectory"
  -G Ninja
  -DCMAKE_C_COMPILER="$C_COMPILER"
  -DCMAKE_CXX_COMPILER="$CXX_COMPILER"
  -DCMAKE_BUILD_TYPE="$BuildType"
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if $NoUnityBuild; then
  CMAKE_ARGS+=(-DXII_ENABLE_FOLDER_UNITY_FILES=OFF)
else
  CMAKE_ARGS+=(-DXII_ENABLE_FOLDER_UNITY_FILES=ON)
fi

if [[ "$VulkanSupport" == "off" ]]; then
  CMAKE_ARGS+=(-DXII_BUILD_VULKAN=OFF)
else
  CMAKE_ARGS+=(-DXII_BUILD_VULKAN=ON)
fi

# D3D12 is ignored on Linux but kept for parity
if [[ "$D3D12Support" == "off" ]]; then
  CMAKE_ARGS+=(-DXII_BUILD_D3D12=OFF)
else
  CMAKE_ARGS+=(-DXII_BUILD_D3D12=ON)
fi

if [[ -n "$SolutionName" ]]; then
  CMAKE_ARGS+=("-XII_SOLUTION_NAME=$SolutionName")
fi

# ----------------------------------------
# Run CMake
# ----------------------------------------
echo "Using workspace directory: Workspace/$WorkspaceDirectory"
echo ""
echo "Running CMake with:"
printf '  %s\n' "${CMAKE_ARGS[@]}"
echo ""

cmake "${CMAKE_ARGS[@]}"

echo ""
echo "Run: ninja -C Workspace/$WorkspaceDirectory"
