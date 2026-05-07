<#
qt-build-relative.ps1
Windows-native PowerShell script to build Qt from source with relative paths.
Usage:
  # Open "x64 Native Tools PowerShell for VS 20xx"
  cd C:\path\to\project
  .\qt-build-relative.ps1 -EngineConfig Dev -Jobs 8
#>
param(
  [ValidateSet("Debug","Dev","Shipping")]
  [string]$EngineConfig = "Dev",
  [string]$QtVersion = "6.11.0",
  [int]$Jobs = 4,
  [string]$SrcArchive = "",
  [switch]$BuildFromSource
)

Set-StrictMode -Version Latest
$Root = (Get-Location).Path
$SrcRoot = Join-Path $Root "src"
$BuildRoot = Join-Path $Root "build"
$InstallRoot = Join-Path $Root "install"
$Artifacts = Join-Path $Root "artifacts"
$Log = Join-Path $Root "qt-build.log"

New-Item -ItemType Directory -Force -Path $SrcRoot,$BuildRoot,$InstallRoot,$Artifacts | Out-Null

function Map-Config {
  param($Engine)
  switch ($Engine) {
    "Debug" { return @{ CMakeType="Debug"; ConfigureFlags = "-debug" } }
    "Dev"   { return @{ CMakeType="RelWithDebInfo"; ConfigureFlags = "-release -force-debug-info" } }
    "Shipping" { return @{ CMakeType="Release"; ConfigureFlags = "-release" } }
  }
}

$cfg = Map-Config -Engine $EngineConfig
$CMakeType = $cfg.CMakeType
$ConfigureFlags = $cfg.ConfigureFlags

Write-Output "Starting Qt build helper in $Root"
Write-Output "EngineConfig: $EngineConfig -> CMake: $CMakeType"

# Ensure required tools
$required = @("cmake","ninja","python","7z")
foreach ($r in $required) {
  if (-not (Get-Command $r -ErrorAction SilentlyContinue)) {
    throw "Missing required tool: $r. Install and add to PATH."
  }
}

# Locate vcvarsall.bat
function Find-Vcvars {
  $vswhere = (Get-Command vswhere.exe -ErrorAction SilentlyContinue)
  if ($vswhere) {
    $inst = & vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
    if ($inst) { return Join-Path $inst "VC\Auxiliary\Build\vcvarsall.bat" }
  }
  $candidates = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio","C:\Program Files\Microsoft Visual Studio" -Recurse -Filter vcvarsall.bat -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($candidates) { return $candidates.FullName }
  return $null
}

$vcvars = Find-Vcvars
if (-not $vcvars -and -not (Get-Command cl -ErrorAction SilentlyContinue)) {
  throw "MSVC not found. Run this script from 'x64 Native Tools PowerShell' or ensure vcvarsall.bat is available."
}

# Prepare source
if ($BuildFromSource -or -not (Get-Command qt6  -ErrorAction SilentlyContinue)) {
  if (-not $SrcArchive) { $SrcArchive = Join-Path $SrcRoot "qt-everywhere-src-$QtVersion.zip" }
  if (-not (Test-Path (Join-Path $SrcRoot "qt-everywhere-src"))) {
    if (-not (Test-Path $SrcArchive)) { throw "Source archive not found at $SrcArchive" }
    & 7z x $SrcArchive -o$SrcRoot | Out-Null
    # normalize folder name
    $ex = Get-ChildItem $SrcRoot -Directory | Where-Object { $_.Name -like "qt-everywhere*" } | Select-Object -First 1
    if ($ex) { Rename-Item $ex.FullName (Join-Path $SrcRoot "qt-everywhere-src") -Force }
  }
}

# Build via a temporary .cmd so vcvars is applied
$buildDir = Join-Path $BuildRoot "qt-build"
if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Path $buildDir | Out-Null

$winPrefix = (Join-Path $InstallRoot $QtVersion) + "\" + $CMakeType
$srcDir = Join-Path $SrcRoot "qt-everywhere-src"

$cmdFile = Join-Path $env:TEMP ("qt_build_" + [guid]::NewGuid().ToString() + ".cmd")
$cmdContent = @"
@echo off
CALL "$vcvars" amd64 2>nul || echo "vcvarsall not called; ensure you are in a Developer Prompt"
pushd "%~dp0"
pushd "$srcDir"
configure.bat -prefix "$winPrefix" $ConfigureFlags -nomake examples -nomake tests
popd
cmake --build . --config $CMakeType -- /m:$Jobs
cmake --install . --config $CMakeType --prefix "$winPrefix"
popd
"@
Set-Content -Path $cmdFile -Value $cmdContent -Encoding ASCII

Write-Output "Running build script: $cmdFile"
& cmd.exe /c $cmdFile
Remove-Item $cmdFile -Force

if ($EngineConfig -eq "Shipping") {
  $installPath = $winPrefix -replace "\\","/"
  $dbgDir = Join-Path $installPath "debug-symbols"
  New-Item -ItemType Directory -Force -Path $dbgDir | Out-Null
  Get-ChildItem -Path $installPath -Recurse -Filter *.pdb -ErrorAction SilentlyContinue | Move-Item -Destination $dbgDir -Force
  Write-Output "PDBs moved to $dbgDir"
}

Write-Output "Build complete. Install tree: $InstallRoot\$QtVersion\$CMakeType"
