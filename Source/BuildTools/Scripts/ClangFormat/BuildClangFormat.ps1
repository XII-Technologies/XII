<#
.SYNOPSIS
  Clone, build, and package clang-format in the current directory.
#>

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$SrcDir = ".\llvm-project"
$BuildDir = ".\llvm-build"
$ToolsDir = ".\tools\precompiled"
$InstallDir = "$HOME\.local\llvm"  # Optional

function Cleanup {
  Write-Host "Cleaning previous build artifacts..."
  Remove-Item -Recurse -Force -ErrorAction SilentlyContinue @($SrcDir, $BuildDir)
}

function Build-ClangFormat {
  Write-Host "Cloning LLVM source..."
  git clone https://github.com/llvm/llvm-project.git $SrcDir

  if (Get-Command ninja.exe -ErrorAction SilentlyContinue) {
    $gen = "Ninja"; $arch = ""
  }
  else {
    $gen = "Visual Studio 17 2022"; $arch = "-A x64"
  }
  Write-Host "Using CMake generator: $gen $arch"

  cmake -G $gen $arch `
    -S "$SrcDir\llvm" `
    -B $BuildDir `
    -DCMAKE_BUILD_TYPE=Release `
    -DLLVM_ENABLE_PROJECTS="clang" `
    -DLLVM_TARGETS_TO_BUILD="X86" `
    -DLLVM_ENABLE_ASSERTIONS=ON

  Write-Host "Building clang-format..."
  if ($gen -eq "Ninja") {
    ninja -C $BuildDir clang-format
  }
  else {
    cmake --build $BuildDir --config Release --target clang-format
  }
}

function Package-Binary {
  Write-Host "Packaging clang-format.exe..."
  $exe = Get-ChildItem -Path $BuildDir -Filter clang-format.exe -Recurse | Select-Object -First 1
  New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
  Copy-Item -Force $exe.FullName $ToolsDir
}

function Add-ToPath {
  Write-Host "Adding clang-format to user PATH at $InstallDir\bin..."
  $exe = Get-ChildItem -Path $BuildDir -Filter clang-format.exe -Recurse | Select-Object -First 1
  New-Item -ItemType Directory -Force -Path "$InstallDir\bin" | Out-Null
  Copy-Item -Force $exe.FullName "$InstallDir\bin\"
  $old = [Environment]::GetEnvironmentVariable("Path", "User")
  if (-not ($old.Split(';') -contains "$InstallDir\bin")) {
    [Environment]::SetEnvironmentVariable("Path", "$InstallDir\bin;$old", "User")
    Write-Host "User PATH updated. Restart PowerShell to apply."
  }
  else { Write-Host "InstallDir already in PATH." }
}

# Main execution
Cleanup
Build-ClangFormat
Package-Binary

# To enable direct CLI usage, uncomment:
# Add-ToPath

Write-Host "clang-format build complete. Binary at $ToolsDir\clang-format.exe"
