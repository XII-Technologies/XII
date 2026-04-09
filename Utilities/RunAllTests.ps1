#!/usr/bin/env powershell
param
(
    [Parameter(Mandatory = $True, HelpMessage="Which build to use.")]
    [ValidateSet('Debug', 'Dev', 'Shipping')][string] $BuildType,
    [switch] $SkipFoundationTest,
    [switch] $SkipCoreTest,
    [switch] $SkipToolsFoundationTest,
    [switch] $SkipGraphicsTest
)

$Path = "$PSScriptRoot/../Output/Bin/WinVs2026$($BuildType)64"

function RunTest($name) {

    Write-Host "`nRunning $name.`n" -ForegroundColor Yellow

    & "$Path\$name.exe" -nosave -all -nogui

    if (!$?) {
        Write-Host "`n$name failed`n" -ForegroundColor Yellow
        throw
    }

    Write-Host "`n$name succeeded.`n" -ForegroundColor Green
}

if (-not $SkipFoundationTest) {
    RunTest "FoundationTest"
}

if (-not $SkipCoreTest) {
    RunTest "CoreTest"
}

if (-not $SkipToolsFoundationTest) {
    RunTest "ToolsFoundationTest"
}

if (-not $SkipGraphicsTest) {
    RunTest "GraphicsTest"
}
