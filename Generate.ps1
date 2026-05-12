param
(
  [Parameter(Mandatory = $True)]
  [ValidateSet('Win64Ninja', 'Win32Ninja', 'Win64vs2026', 'Win32vs2026', 'Win64vs2022', 'Win32vs2022')]
  [string] $Target,

  [switch]$NoUnityBuild,
  [switch]$NoSubmoduleUpdate,
  [string]$SolutionName = "",
  [string]$WorkspaceDirectory = "",
  [ValidateSet('Debug', 'Dev', 'Shipping')][string] $Configuration = 'Dev',
  [switch]$Build,
  [int]$Jobs = [int]([Environment]::ProcessorCount),
  [Nullable[bool]]$D3D12Support,
  [Nullable[bool]]$VulkanSupport
)

Set-Location $PSScriptRoot

# Submodule update
if (-not $NoSubmoduleUpdate)
{
  $CURRENT_COMMIT = git log -n 1 --format=%H
  Write-Host "Current commit: $CURRENT_COMMIT"

  $UPDATE_SUBMODULES = $True
  $LAST_UPDATE_FILE = "$PSScriptRoot\Data\Content\AssetCache\LastSubmoduleUpdate.txt"

  if (Test-Path $LAST_UPDATE_FILE -PathType Leaf -ErrorAction SilentlyContinue)
  {
    $LAST_COMMIT = Get-Content -Path $LAST_UPDATE_FILE
    if ($CURRENT_COMMIT -eq $LAST_COMMIT)
    {
      Write-Host "Submodules already up-to-date."
      $UPDATE_SUBMODULES = $False
    }
    else
    {
      Write-Host "Submodules were last updated at commit: $LAST_COMMIT"
    }
  }

  if ($UPDATE_SUBMODULES)
  {
    Write-Host "Updating submodules" -ForegroundColor Green
    git submodule init
    git submodule update --recursive
    Out-File ( New-Item -Path $LAST_UPDATE_FILE -Force) -InputObject $CURRENT_COMMIT
  }
}

# Base CMake args
$CMAKE_ARGS = @("-S", "$PSScriptRoot")

if ($NoUnityBuild) { $CMAKE_ARGS += "-DXII_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF" }
else { $CMAKE_ARGS += "-DXII_ENABLE_FOLDER_UNITY_FILES:BOOL=ON" }

if ($VulkanSupport.HasValue -and $VulkanSupport.Value -eq $False) { $CMAKE_ARGS += "-DXII_BUILD_VULKAN:BOOL=OFF" }
else { $CMAKE_ARGS += "-DXII_BUILD_VULKAN:BOOL=ON" }

if ($D3D12Support.HasValue -and $D3D12Support.Value -eq $True) { $CMAKE_ARGS += "-DXII_BUILD_D3D12:BOOL=ON" }
else { $CMAKE_ARGS += "-DXII_BUILD_D3D12:BOOL=OFF" }

if ($SolutionName -ne "") { $CMAKE_ARGS += "-DXII_SOLUTION_NAME:STRING='$SolutionName'" }

# Workspace directory handling
$IsCustomWorkspaceDirectory = $False
if ($WorkspaceDirectory -ne "") { $IsCustomWorkspaceDirectory = $True }

$UseVisualStudioGenerator = $False

# Ninja-first logic (Ninja Multi-Config is default)
if ($Target -match 'Ninja$')
{
  Write-Host "=== Generating Ninja Multi-Config build files for target $Target ==="

  if ($Target -like 'Win64*') { $Arch = 'x64'; $WorkspaceName = 'Win64Ninja' }
  elseif ($Target -like 'Win32*') { $Arch = 'Win32'; $WorkspaceName = 'Win32Ninja' }
  else { throw "Unknown architecture in target '$Target'." }

  # Use Ninja Multi-Config generator (requires CMake 3.17+). This allows building Debug/Dev/Shipping from the same generated files, similar to Visual Studio.
  $CMAKE_ARGS += "-G"; $CMAKE_ARGS += "Ninja Multi-Config"

  # Expose configuration names and set the default.
  $CMAKE_ARGS += "-DCMAKE_CONFIGURATION_TYPES=Debug;Dev;Shipping"
  $CMAKE_ARGS += "-DCMAKE_DEFAULT_BUILD_TYPE=$Configuration"

  # Also set CMAKE_BUILD_TYPE in the cache so legacy checks that read it get the expected value.
  # This does not break multi-config behavior and helps projects that still query CMAKE_BUILD_TYPE.
  $CMAKE_ARGS += "-DCMAKE_BUILD_TYPE:STRING=$Configuration"

  # Platform and explicit config variables.
  $CMAKE_ARGS += "-DCMAKE_GENERATOR_PLATFORM=$Arch"

  # Use clang-cl for MSVC ABI compatibility.
  $CMAKE_ARGS += "-DCMAKE_C_COMPILER=clang-cl"
  $CMAKE_ARGS += "-DCMAKE_CXX_COMPILER=clang-cl"

  if (-not $IsCustomWorkspaceDirectory) { $WorkspaceDirectory = $WorkspaceName }
}
elseif ($Target -match 'vs2026$' -or $Target -match 'vs2022$')
{
  if ($Target -like 'Win64vs2026') { $Arch = 'x64'; $GeneratorName = 'Visual Studio 18 2026'; $WorkspaceName = 'Win64vs2026' }
  elseif ($Target -like 'Win32vs2026') { $Arch = 'Win32'; $GeneratorName = 'Visual Studio 18 2026'; $WorkspaceName = 'Win32vs2026' }
  elseif ($Target -like 'Win64vs2022') { $Arch = 'x64'; $GeneratorName = 'Visual Studio 17 2022'; $WorkspaceName = 'Win64vs2022' }
  elseif ($Target -like 'Win32vs2022') { $Arch = 'Win32'; $GeneratorName = 'Visual Studio 17 2022'; $WorkspaceName = 'Win32vs2022' }
  else { throw "Unknown Visual Studio target '$Target'." }

  Write-Host "=== Generating Solution for $GeneratorName $Arch ==="

  $CMAKE_ARGS += "-G"; $CMAKE_ARGS += $GeneratorName
  $CMAKE_ARGS += "-A"; $CMAKE_ARGS += $Arch

  if (-not $IsCustomWorkspaceDirectory) { $WorkspaceDirectory = $WorkspaceName }

  $UseVisualStudioGenerator = $True
}
else
{
  throw "Unknown target '$Target'."
}

# Add build directory.
if (-not ($CMAKE_ARGS -contains "-B"))
{
  $CMAKE_ARGS += "-B"
  $CMAKE_ARGS += "$PSScriptRoot\Workspace\$WorkspaceDirectory"
}

Write-Host "Using workspace directory: $PSScriptRoot\Workspace\$WorkspaceDirectory"

# Custom output directories.
if ($IsCustomWorkspaceDirectory)
{
  $CMAKE_ARGS += "-DXII_OUTPUT_DIRECTORY_DLL:PATH=$PSScriptRoot\Workspace\$WorkspaceDirectory-output\Bin"
  $CMAKE_ARGS += "-DXII_OUTPUT_DIRECTORY_LIB:PATH=$PSScriptRoot\Workspace\$WorkspaceDirectory-output\Lib"
  Write-Host "Custom output directories: Workspace\$WorkspaceDirectory-output\"
}

# Warn if clang-cl missing when using Ninja.
if ($CMAKE_ARGS -contains "-DCMAKE_C_COMPILER=clang-cl")
{
  $clangPath = (Get-Command clang-cl -ErrorAction SilentlyContinue).Path
  if (-not $clangPath)
  {
    Write-Warning "clang-cl not found on PATH. Ensure clang-cl is installed and on PATH."
  }
  else
  {
    Write-Host "Found clang-cl at: $clangPath"
  }
}

Write-Host ""
Write-Host "Running cmake.exe $CMAKE_ARGS" -ForegroundColor Green
Write-Host ""
&Data\Tools\Precompiled\cmake\bin\cmake.exe $CMAKE_ARGS

if (!$?) { throw "CMake failed with exit code '$LASTEXITCODE'." }

# Build step: always use --config <Configuration> so both VS and Ninja Multi-Config behave the same.
if ($Build)
{
  Write-Host ""
  Write-Host "Starting build (Configuration = $Configuration, Jobs = $Jobs)" -ForegroundColor Green
  $BuildDir = "$PSScriptRoot\Workspace\$WorkspaceDirectory"

  if ($UseVisualStudioGenerator)
  {
    # Visual Studio: multi-config, use MSBuild parallel switch.
    cmake --build $BuildDir --config $Configuration -- /m:$Jobs
    if (!$?) { throw "Build failed with exit code '$LASTEXITCODE'." }
  }
  else
  {
    # Ninja Multi-Config: cmake --build supports --config, we pass -j to ninja via the -- separator.
    cmake --build $BuildDir --config $Configuration -- -j $Jobs
    if (!$?) { throw "Build failed with exit code '$LASTEXITCODE'." }
  }

  Write-Host "Build completed successfully." -ForegroundColor Green
}
