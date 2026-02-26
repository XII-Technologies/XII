param
(
  [Parameter(Mandatory = $True)] [ValidateSet('Win64vs2026', 'Win64vs2022')][string] $Target,
  [switch]$NoUnityBuild,
  [switch]$NoSubmoduleUpdate,
  [string]$SolutionName,
  [string]$WorkspaceDirectory
)

Set-Location $PSScriptRoot

if ($NoSubmoduleUpdate -eq $False) {
  $CURRENT_COMMIT = git log -n 1 --format=%H

  Write-Host "Current commit: $CURRENT_COMMIT"

  $UPDATE_SUBMODULES = $True
  $LAST_UPDATE_FILE = "$PSScriptRoot\Data\Content\AssetCache\LastSubmoduleUpdate.txt"

  if (Test-Path $LAST_UPDATE_FILE -PathType Leaf -ErrorAction SilentlyContinue) {
    $LAST_COMMIT = Get-Content -Path $LAST_UPDATE_FILE

    if ($CURRENT_COMMIT -eq $LAST_COMMIT) {
      Write-Host "Submodules already up-to-date."
      $UPDATE_SUBMODULES = $False
    }
    else {
      Write-Host "Submodules were last updated at commit: $LAST_COMMIT"
    }
  }

  if ($UPDATE_SUBMODULES) {
    Write-Host "Updating submodules" -ForegroundColor Green

    git submodule init
    git submodule update

    Out-File ( New-Item -Path $LAST_UPDATE_FILE -Force) -InputObject $CURRENT_COMMIT
  }
}

$CMAKE_ARGS = @("-S", "$PSScriptRoot")

if ($NoUnityBuild) {
  $CMAKE_ARGS += "-DXII_ENABLE_FOLDER_UNITY_FILES:BOOL=OFF"
}
else {
  $CMAKE_ARGS += "-DXII_ENABLE_FOLDER_UNITY_FILES:BOOL=ON"
}

if ($SolutionName -ne "") {
  $CMAKE_ARGS += "-XII_SOLUTION_NAME:STRING='$SolutionName'"
}

$CMAKE_ARGS += "-DXII_BUILD_VULKAN:BOOL=ON"

$CMAKE_ARGS += "-G"

Write-Host ""

$IsCustomWorkspaceDirectory = $False
if ($WorkspaceDirectory -ne "") {
  $IsCustomWorkspaceDirectory = $True
}

if ($Target -eq "Win64vs2026") {

  Write-Host "=== Generating Solution for Visual Studio 2026 x64 ==="

  $CMAKE_ARGS += "Visual Studio 18 2026"
  $CMAKE_ARGS += "-A"
  $CMAKE_ARGS += "x64"

  if (-not $IsCustomWorkspaceDirectory) {
    $WorkspaceDirectory = "vs2026x64"
  }
}
elseif ($Target -eq "Win64vs2022") {

  Write-Host "=== Generating Solution for Visual Studio 2022 x64 ==="

  $CMAKE_ARGS += "Visual Studio 17 2022"
  $CMAKE_ARGS += "-A"
  $CMAKE_ARGS += "x64"
  $CMAKE_ARGS += "-B"
  $CMAKE_ARGS += "$PSScriptRoot\Workspace\vs2022x64"

  if (-not $IsCustomWorkspaceDirectory) {
    $WorkspaceDirectory = "vs2022x64"
  }
}
else {
  throw "Unknown target '$Target'."
}

# Add build directory to cmake arguments.
$CMAKE_ARGS += "-B"
$CMAKE_ARGS += "$PSScriptRoot\Workspace\$WorkspaceDirectory"

Write-Host "Using workspace directory: $PSScriptRoot\Workspace\$WorkspaceDirectory"

# Set custom output directories to avoid conflicts between different build targets.
if ($IsCustomWorkspaceDirector) {
  $CMAKE_ARGS += "-DXII_OUTPUT_DIRECTORY_DLL:PATH=$PSScriptRoot\Workspace\$WorkspaceDirectory-output\Bin"
  $CMAKE_ARGS += "-DXII_OUTPUT_DIRECTORY_LIB:PATH=$PSScriptRoot\Workspace\$WorkspaceDirectory-output\Lib"

  Write-Host "Custom output directories: Workspace\$WorkspaceDirectory-output\"
}

Write-Host ""
Write-Host "Running cmake.exe $CMAKE_ARGS" -ForegroundColor Green
Write-Host ""
&Data\Tools\Precompiled\cmake\bin\cmake.exe $CMAKE_ARGS

if (!$?) {
  throw "CMake failed with exit code '$LASTEXITCODE'."
}
