param(
  [string]$QtVersion = "v6.11.0",
  [string]$RepoUrl = "https://code.qt.io/qt/qt5.git",
  [int]$CloneDepth = 1,
  [int]$Jobs = [Environment]::ProcessorCount
)

$ErrorActionPreference = 'Stop'
$Root = (Get-Location).ProviderPath
$LogFile = Join-Path $Root "qt-build-powershell.log"

Start-Transcript -Path $LogFile -Force

try
{
  function Log { param($m) Write-Output $m }

  # ------------------------------------------------------------
  # 1. Resolve REAL Git
  # ------------------------------------------------------------
  function Resolve-RealGit
  {
    $candidates = @(
      "C:\Program Files\Git\cmd\git.exe",
      "C:\Program Files\Git\bin\git.exe",
      "C:\Program Files (x86)\Git\cmd\git.exe"
    )

    foreach ($c in $candidates)
    {
      if (Test-Path $c)
      {
        # Validate it is real Git
        $ver = & $c --version 2>$null
        if ($ver -match "git version")
        {
          return $c
        }
      }
    }

    # Fallback: search PATH but filter out known bad locations
    $all = (Get-Command git.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)
    foreach ($p in $all)
    {
      if ($p -match "Program Files\\Git" -and -not ($p -match "GitHub" -or $p -match "Qt" -or $p -match "Microsoft Visual Studio"))
      {
        return $p
      }
    }

    throw "Could not locate a valid Git for Windows installation."
  }

  $Git = Resolve-RealGit
  Log "Using Git: $Git"

  # Resolve cmake and ninja absolutely
  function Resolve-Tool($name)
  {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $cmd) { throw "Missing required tool: $name" }
    return $cmd.Source
  }

  $CMake = Resolve-Tool "cmake"
  $Ninja = Resolve-Tool "ninja"

  Log "Using CMake: $CMake"
  Log "Using Ninja: $Ninja"

  # ------------------------------------------------------------
  # 2. Locate vcvars64.bat
  # ------------------------------------------------------------
  function Find-Vcvars64
  {
    if ($env:VSINSTALLDIR)
    {
      $candidate = Join-Path $env:VSINSTALLDIR "VC\Auxiliary\Build\vcvars64.bat"
      if (Test-Path $candidate) { return $candidate }
    }

    if (Get-Command vswhere -ErrorAction SilentlyContinue)
    {
      $vsPath = & vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
      if ($vsPath)
      {
        $candidate = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $candidate) { return $candidate }
      }
    }

    $fallbacks = @(
      "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
      "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
      "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($p in $fallbacks) { if (Test-Path $p) { return $p } }
    return $null
  }

  $vcvars64 = Find-Vcvars64
  if (-not $vcvars64) { throw "vcvars64.bat not found." }
  Log "Found vcvars64: $vcvars64"

  # ------------------------------------------------------------
  # 3. Paths
  # ------------------------------------------------------------
  $SrcRoot = Join-Path $Root "src"
  $SrcDir = Join-Path $SrcRoot "qt"
  $InstallDir = Join-Path $Root "install\qt"

  # ------------------------------------------------------------
  # 4. Clone Qt repository
  # ------------------------------------------------------------
  if (-not (Test-Path $SrcDir))
  {
    New-Item -ItemType Directory -Force -Path $SrcRoot | Out-Null

    $tmpClone = Join-Path $SrcRoot ("qt-clone-tmp-{0}" -f ([guid]::NewGuid()))
    New-Item -ItemType Directory -Force -Path $tmpClone | Out-Null

    Log "Cloning Qt from $RepoUrl (branch $QtVersion, depth $CloneDepth)"

    $gitArgs = @("clone")

    if ($QtVersion)
    {
      $gitArgs += "--branch"
      $gitArgs += $QtVersion
    }

    if ($CloneDepth -gt 0)
    {
      $gitArgs += "--depth"
      $gitArgs += $CloneDepth.ToString()
      $gitArgs += "--shallow-submodules"
    }

    $gitArgs += "--recurse-submodules"
    $gitArgs += $RepoUrl
    $gitArgs += $tmpClone

    Log "Running: $Git $($gitArgs -join ' ')"

    $proc = Start-Process -FilePath $Git -ArgumentList $gitArgs -NoNewWindow -Wait -PassThru
    if ($proc.ExitCode -ne 0)
    {
      Remove-Item -Recurse -Force $tmpClone -ErrorAction SilentlyContinue
      throw "git clone failed with exit code $($proc.ExitCode)"
    }

    Move-Item -Path $tmpClone -Destination $SrcDir -Force
    Log "Clone complete: $SrcDir"
  }
  else
  {
    Log "Source already present at $SrcDir; skipping clone"
  }

  # ------------------------------------------------------------
  # 5. Initialize submodules (qtbase + qtsvg)
  # ------------------------------------------------------------
  Push-Location $SrcDir
  try
  {
    $initScript = Join-Path $SrcDir "init-repository.bat"
    if (-not (Test-Path $initScript))
    {
      throw "init-repository.bat missing in $SrcDir"
    }

    $subset = "qtbase,qtsvg"
    Log "Initializing submodules: $subset"

    $initProc = Start-Process -FilePath $initScript -ArgumentList "--module-subset=$subset" -NoNewWindow -Wait -PassThru
    if ($initProc.ExitCode -ne 0)
    {
      throw "init-repository.bat failed with exit code $($initProc.ExitCode)"
    }

    Log "Submodules initialized."
  }
  finally
  {
    Pop-Location
  }

  # ------------------------------------------------------------
  # 6. Build Qt (debug+release)
  # ------------------------------------------------------------
  New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

  $configureArgs = '-submodules qtsvg,qtbase -nomake examples -nomake tests -prefix "' + $InstallDir + '" -debug-and-release -force-debug-info'

  $batchFile = Join-Path $env:TEMP ("qt-build-{0}.cmd" -f $PID)
  $batchContent = @"
@echo off
call "$vcvars64" amd64
if errorlevel 1 exit /b %ERRORLEVEL%
pushd "$SrcDir"
if errorlevel 1 exit /b %ERRORLEVEL%
call "$SrcDir\configure.bat" $configureArgs
if errorlevel 1 exit /b %ERRORLEVEL%
"$CMake" -G "Ninja" -S "$SrcDir" -B "$SrcDir" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$InstallDir"
if errorlevel 1 exit /b %ERRORLEVEL%
"$CMake" --build "$SrcDir" --parallel $Jobs
if errorlevel 1 exit /b %ERRORLEVEL%
"$CMake" --install "$SrcDir" --prefix "$InstallDir"
if errorlevel 1 exit /b %ERRORLEVEL%
popd
exit /b 0
"@

  Set-Content -Path $batchFile -Value $batchContent -Encoding ASCII

  Log "Running build batch: $batchFile"
  & cmd /c $batchFile
  $rc = $LASTEXITCODE
  Remove-Item $batchFile -ErrorAction SilentlyContinue

  if ($rc -ne 0)
  {
    throw "Build failed with exit code $rc"
  }

  Log "Build finished. Installed to $InstallDir"

}
finally
{
  try { Stop-Transcript } catch { }
}
