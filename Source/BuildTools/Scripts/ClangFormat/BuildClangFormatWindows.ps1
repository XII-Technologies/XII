<#
.SYNOPSIS
    Clone LLVM, build clang-format, and package the binary on Windows.

.DESCRIPTION
    - Cleans up previous build artifacts
    - Clones https://github.com/llvm/llvm-project
    - Configures and builds only clang-format (Ninja if available; otherwise VS)
    - Copies the resulting clang-format.exe into a “precompiled tools” folder
    - Provides an optional function to add clang-format to your user PATH

.PARAMETER SrcDir
    Path to clone llvm-project into (default: $HOME\src\llvm-project).

.PARAMETER BuildDir
    Path to configure and build LLVM (default: $HOME\build\llvm).

.PARAMETER ToolsDir
    Destination for the packaged clang-format.exe (default: $HOME\tools\precompiled).

.PARAMETER InstallDir
    (Optional) Local “install” prefix if you want to add clang-format to your PATH
    (default: $HOME\.local\llvm).

.EXAMPLE
    .\build-clang-format.ps1
#>

[CmdletBinding()]
param(
    [string]$SrcDir     = "$HOME\src\llvm-project",
    [string]$BuildDir   = "$HOME\build\llvm",
    [string]$ToolsDir   = "$HOME\tools\precompiled",
    [string]$InstallDir = "$HOME\.local\llvm"
)

$ErrorActionPreference = 'Stop'

function Cleanup {
    Write-Host "Cleaning previous build artifacts..."
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue @($SrcDir, $BuildDir)
}

function Build-ClangFormat {
    Write-Host "Cloning LLVM project into `"$SrcDir`"..."
    git clone https://github.com/llvm/llvm-project.git $SrcDir

    Write-Host "Detecting build tool..."
    if (Get-Command ninja.exe -ErrorAction SilentlyContinue) {
        $generator = "Ninja"
        $archArg   = ""
    }
    else {
        $generator = "Visual Studio 17 2022"
        $archArg   = "-A x64"
    }
    Write-Host "Using CMake generator: $generator $archArg"

    Write-Host "Configuring build in `"$BuildDir`"..."
    cmake -G $generator $archArg `
        -S "$SrcDir\llvm" `
        -B $BuildDir `
        -DCMAKE_BUILD_TYPE=Release `
        -DLLVM_ENABLE_PROJECTS="clang" `
        -DLLVM_TARGETS_TO_BUILD="X86" `
        -DLLVM_ENABLE_ASSERTIONS=ON

    Write-Host "Building clang-format..."
    if ($generator -eq "Ninja") {
        ninja -C $BuildDir clang-format
    }
    else {
        cmake --build $BuildDir --config Release --target clang-format
    }
}

function Package-Binary {
    Write-Host "Locating clang-format.exe..."
    $exe = Get-ChildItem -Path $BuildDir -Filter clang-format.exe -Recurse -ErrorAction Stop |
           Select-Object -First 1

    Write-Host "Copying clang-format.exe to `"$ToolsDir`"..."
    New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
    Copy-Item -Force $exe.FullName $ToolsDir
}

function Add-ToPath {
    Write-Host "Adding clang-format to your user PATH via `"$InstallDir\bin`"..."
    $exe = Get-ChildItem -Path $BuildDir -Filter clang-format.exe -Recurse -ErrorAction Stop |
           Select-Object -First 1

    New-Item -ItemType Directory -Force -Path "$InstallDir\bin" | Out-Null
    Copy-Item -Force $exe.FullName "$InstallDir\bin\"

    $oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not ($oldPath.Split(';') -contains "$InstallDir\bin")) {
        $newPath = "$InstallDir\bin;$oldPath"
        [Environment]::SetEnvironmentVariable("Path", $newPath, "User")
        Write-Host "User PATH updated. Restart your PowerShell session to apply changes."
    }
    else {
        Write-Host "InstallDir already in user PATH."
    }
}

# Main execution
Cleanup
Build-ClangFormat
Package-Binary

# To enable direct CLI usage, uncomment the following line:
# Add-ToPath

Write-Host "clang-format build complete. Binary is available at $ToolsDir\clang-format.exe"
