<#
.SYNOPSIS
  Minimal Qt 6 build helper for Windows using Ninja and MSVC.

.DESCRIPTION
  - Downloads (aria2 / BITS / Invoke-WebRequest) or uses local archive.
  - Extracts Qt sources to ./src.
  - Runs configure.bat once (single-line) inside a cmd session that sources vcvars64.bat.
  - Runs cmake -G "Ninja", builds and installs (in-source).
  - Logs to qt-build-powershell.log.

USAGE
  Open PowerShell and run:
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\build-qt.ps1 -QtVersion 6.11.0 -Jobs 8
#>

param(
  [string]$QtVersion = "6.11.0",
  [string]$SrcArchive = "",
  [int]$Jobs = [Environment]::ProcessorCount,
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
  if (-not (Test-Path $SrcArchive)) {
    Stop-Transcript
    throw "Provided archive path '$SrcArchive' does not exist."
  }
  $ArchivePath = (Resolve-Path $SrcArchive).ProviderPath
}
else {
  $ArchivePath = Join-Path $SrcRoot $ArchiveNameZip
}

$SrcDir = Join-Path $SrcRoot "qt-everywhere-src"
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
$pythonCmd = if (Get-Command python -ErrorAction SilentlyContinue) { "python" }
elseif (Get-Command py -ErrorAction SilentlyContinue) { "py" }
else { $null }

if (-not $pythonCmd) {
  Stop-Transcript
  throw "Missing required tool: python (or py). Install Python and ensure it's in PATH."
}

foreach ($t in @("cmake", "ninja")) {
  if (-not (Get-Command $t -ErrorAction SilentlyContinue)) {
    Stop-Transcript
    throw "Missing required tool: $t. Install and ensure it's in PATH."
  }
}

# Locate vcvars64.bat robustly (use vswhere if present)
function Find-Vcvars64 {
  # Try VSINSTALLDIR first
  if ($env:VSINSTALLDIR) {
    $candidate = Join-Path $env:VSINSTALLDIR "VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $candidate) { return $candidate }
  }

  # Try vswhere if available
  if (Get-Command vswhere -ErrorAction SilentlyContinue) {
    try {
      $vsPath = & vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
      if ($vsPath) {
        $candidate = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $candidate) { return $candidate }
      }
    }
    catch { }
  }

  # Common fallback locations (cover VS 2022/2026 Community/Professional/Enterprise)
  $possible = @(
    "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2026\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2026\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  )
  foreach ($p in $possible) { if (Test-Path $p) { return $p } }
  return $null
}

$vcvars64 = Find-Vcvars64
if (-not $vcvars64) {
  Stop-Transcript
  throw "vcvars64.bat not found. Run this script from Developer PowerShell (x64) or install Visual Studio and ensure vcvars64.bat is available."
}
Log "Found vcvars64: $vcvars64"

# Ensure source archive present or download and extract
if (-not (Test-Path $SrcDir)) {
  New-Item -ItemType Directory -Force -Path $SrcRoot | Out-Null

  if (-not (Test-Path $ArchivePath)) {
    $url = Get-QtDownloadUrl $QtVersion $ArchiveNameZip
    if ($FastBrowserDownload) {
      Log "FAST_DOWNLOAD requested. Download URL:"
      Log $url
      Stop-Transcript
      exit 0
    }
    try {
      Download-File $url $ArchivePath
    }
    catch {
      Log "Primary download failed, trying tar.xz fallback"
      $url2 = Get-QtDownloadUrl $QtVersion $ArchiveNameTar
      $ArchivePath = Join-Path $SrcRoot $ArchiveNameTar
      Download-File $url2 $ArchivePath
    }
  }
  else {
    Log "Using provided archive: $ArchivePath"
  }

  # Extract
  if ($ArchivePath -like "*.zip") {
    Log "Extracting zip $ArchivePath"
    Expand-Archive -Path $ArchivePath -DestinationPath $SrcRoot -Force
  }
  else {
    Log "Extracting tar.xz $ArchivePath"
    if (Get-Command tar -ErrorAction SilentlyContinue) {
      tar -xf $ArchivePath -C $SrcRoot
    }
    else {
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
}
else {
  Log "Source already extracted at $SrcDir"
}

# Build commands to run inside a single cmd.exe session that first calls vcvars64.bat
# We use 'call' so batch files return control to cmd and '&&' to stop on failure.
$winSrc = $SrcDir
$winPrefix = $InstallDir

# Ensure install dir exists
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

# Configure args (adjust as needed)
$configureArgs = "-prefix `"$winPrefix`" -release -nomake examples -nomake tests"

# Build the full cmd script (single-line) to run under cmd /c
# Use double quotes around the whole command for cmd /c, and escape inner quotes properly.
$cmdScript = "call `"$vcvars64`" amd64 && pushd `"$winSrc`" && call `"$winSrc\configure.bat`" $configureArgs && cmake -G `"Ninja`" -S `"$winSrc`" -B `"$winSrc`" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=`"$winPrefix`" && cmake --build `"$winSrc`" --parallel $Jobs && cmake --install `"$winSrc`" --prefix `"$winPrefix`" && popd"

Log "Running configure, build and install inside a single cmd.exe session (this ensures vcvars64 is active for all steps)."
Log "Command: cmd /c <vcvars64 && configure && cmake build && cmake install>"

# Execute the command in cmd.exe so batch files run correctly
& cmd /c $cmdScript
if ($LASTEXITCODE -ne 0) {
  Stop-Transcript
  throw "Build sequence failed with exit code $LASTEXITCODE. See log for details."
}

Stop-Transcript
Log "Build finished. Install tree at $InstallDir"
