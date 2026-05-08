<#
.SYNOPSIS
  Minimal Qt 6 build helper for Windows using Ninja and MSVC.

.DESCRIPTION
  - Downloads (aria2 / BITS / Invoke-WebRequest) or uses local archive.
  - Extracts Qt sources to ./src.
  - Runs configure.bat once (single-line).
  - Runs cmake -G "Ninja", builds and installs.
  - Logs to qt-build-powershell.log.

USAGE
  Open Developer PowerShell (x64) and run:
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\build-qt.ps1 -QtVersion 6.11.0 -Jobs 8

PARAMETERS
  -QtVersion  Qt version string (default 6.11.0)
  -SrcArchive Optional path to local archive
  -Jobs       Parallel build jobs (default 8)
  -FastBrowserDownload If set, prints download URL and exits
#>

param(
  [string]$QtVersion = "6.11.0",
  [string]$SrcArchive = "",
  [int]$Jobs = 8,
  [switch]$FastBrowserDownload
)

$ErrorActionPreference = 'Stop'
$Root = (Get-Location).ProviderPath
$LogFile = Join-Path $Root "qt-build-powershell.log"
Start-Transcript -Path $LogFile -Force

function Log { param($m) Write-Output $m }

# Paths
$SrcRoot = Join-Path $Root "src"
$ArchiveNameZip = "qt-everywhere-src-$QtVersion.zip"
$ArchiveNameTar = "qt-everywhere-src-$QtVersion.tar.xz"
if ($SrcArchive -ne "") {
  $ArchivePath = (Resolve-Path $SrcArchive).ProviderPath
} else {
  $ArchivePath = Join-Path $SrcRoot $ArchiveNameZip
}

$SrcDir = Join-Path $SrcRoot "qt-everywhere-src"
$BuildDir = Join-Path $Root "build\ninja"
$InstallDir = Join-Path $Root "install\qtbase"

# Helpers
function Get-QtDownloadUrl([string]$version, [string]$file) {
  $majorMinor = $version.Substring(0, $version.LastIndexOf('.'))
  return "https://download.qt.io/official_releases/qt/$majorMinor/$version/single/$file"
}

function Download-File($url, $out) {
  New-Item -ItemType Directory -Force -Path (Split-Path $out) | Out-Null

  if (Get-Command aria2c -ErrorAction SilentlyContinue) {
    Log "Using aria2c to download $url"
    & aria2c -x16 -s16 --continue=true -d (Split-Path $out) -o (Split-Path $out -Leaf) $url
    return
  }

  if (Get-Command Start-BitsTransfer -ErrorAction SilentlyContinue) {
    Log "Using Start-BitsTransfer to download $url"
    Start-BitsTransfer -Source $url -Destination $out -Priority Foreground
    return
  }

  Log "Using Invoke-WebRequest to download $url (may be slower)"
  Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing
}

# Ensure required tools
foreach ($t in @("cmake","ninja","python")) {
  if (-not (Get-Command $t -ErrorAction SilentlyContinue)) {
    Stop-Transcript
    throw "Missing required tool: $t. Install and ensure it's in PATH."
  }
}

# Ensure MSVC environment
$vcvars = $null
if ($env:VSCMD_ARG_TGT_ARCH) {
  Log "Detected Developer PowerShell environment."
} else {
  # try to locate vcvarsall.bat via VSINSTALLDIR or common locations
  if ($env:VSINSTALLDIR) {
    $vcvars = Join-Path $env:VSINSTALLDIR "VC\Auxiliary\Build\vcvarsall.bat"
  } else {
    $possible = @(
      "C:\Program Files (x86)\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files (x86)\Microsoft Visual Studio\2026\Professional\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files (x86)\Microsoft Visual Studio\2026\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvarsall.bat"

      "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
      "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    )
    foreach ($p in $possible) { if (Test-Path $p) { $vcvars = $p; break } }
  }
  if (-not $vcvars) {
    Stop-Transcript
    throw "vcvars not found. Run this script from Developer PowerShell or set VSINSTALLDIR."
  } else {
    Log "Sourcing vcvars to ensure MSVC toolchain is available."
    # run vcvars in a cmd child so environment is applied to child only; we need it in this session
    # capture environment and import into PowerShell session
    $envText = cmd /c "`"$vcvars`" amd64 >nul 2>&1 && set"
    $envText -split "`r?`n" | ForEach-Object {
      if ($_ -match '^(.*?)=(.*)$') {
        $name = $matches[1]; $value = $matches[2]
        # avoid overwriting PowerShell-only variables
        Set-Item -Path "Env:$name" -Value $value
      }
    }
  }
}

# Ensure source archive present or download
if (-not (Test-Path $SrcDir)) {
  New-Item -ItemType Directory -Force -Path $SrcRoot | Out-Null

  if (-not (Test-Path $ArchivePath)) {
    # prefer zip on Windows
    $url = Get-QtDownloadUrl $QtVersion $ArchiveNameZip
    if ($FastBrowserDownload) {
      Log "FAST_DOWNLOAD requested. Download URL:"
      Log $url
      Stop-Transcript
      exit 0
    }
    try {
      Download-File $url $ArchivePath
    } catch {
      Log "Primary download failed, trying tar.xz fallback"
      $url2 = Get-QtDownloadUrl $QtVersion $ArchiveNameTar
      Download-File $url2 (Join-Path $SrcRoot $ArchiveNameTar)
      # prefer zip if available; set ArchivePath accordingly
      if (Test-Path (Join-Path $SrcRoot $ArchiveNameTar)) {
        $ArchivePath = Join-Path $SrcRoot $ArchiveNameTar
      }
    }
  } else {
    Log "Using provided archive: $ArchivePath"
  }

  # Extract
  if ($ArchivePath -like "*.zip") {
    Log "Extracting zip $ArchivePath"
    Expand-Archive -Path $ArchivePath -DestinationPath $SrcRoot -Force
  } else {
    Log "Extracting tar.xz $ArchivePath"
    # Use tar if available
    if (Get-Command tar -ErrorAction SilentlyContinue) {
      tar -xf $ArchivePath -C $SrcRoot
    } else {
      Stop-Transcript
      throw "tar not found to extract $ArchivePath"
    }
  }

  # Normalize extracted directory name
  $ex = Get-ChildItem -Path $SrcRoot -Directory | Where-Object { $_.Name -like "qt-everywhere*" } | Select-Object -First 1
  if (-not $ex) {
    Stop-Transcript
    throw "Could not find extracted Qt source directory under $SrcRoot"
  }
  if ($ex.FullName -ne $SrcDir) {
    Move-Item -Path $ex.FullName -Destination $SrcDir -Force
  }
} else {
  Log "Source already extracted at $SrcDir"
}

# Run configure.bat (single-line) under cmd.exe
$winSrc = $SrcDir
$winPrefix = $InstallDir
$configureArgs = "-prefix `"$winPrefix`" -release -nomake examples -nomake tests"
$configureCmd = "`"$winSrc\configure.bat`" $configureArgs"
Log "Running configure.bat (this may take a while)"
$cfgExit = cmd /c $configureCmd
if ($LASTEXITCODE -ne 0) {
  Stop-Transcript
  throw "configure.bat failed with exit code $LASTEXITCODE. See log for details."
}

# Configure CMake for Ninja and build
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Push-Location $BuildDir
try {
  Log "Configuring CMake (Ninja)"
  cmake -G "Ninja" -S $SrcDir -B $BuildDir -D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX=$InstallDir

  Log "Building with Ninja ($Jobs jobs)"
  cmake --build $BuildDir --parallel $Jobs

  Log "Installing to $InstallDir"
  cmake --install $BuildDir --prefix $InstallDir
} catch {
  Pop-Location
  Stop-Transcript
  throw "Build failed: $_"
}
Pop-Location

Stop-Transcript
Log "Build finished. Install tree at $InstallDir"
