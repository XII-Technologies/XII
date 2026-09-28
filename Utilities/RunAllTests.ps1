param
(
  [Parameter(Mandatory = $True, HelpMessage="Which build to use.")]
  [ValidateSet('Debug', 'Dev', 'Shipping')][string] $BuildType,
  [switch] $SkipFoundationTest,
  [switch] $SkipCoreTest,
  [switch] $SkipGraphicsFoundationTest,
  [switch] $SkipTextureTest,
  [switch] $SkipGraphicsCoreTest,
  [switch] $SkipGameEngineTest,
  [switch] $SkipToolsFoundationTest
)

$Path = (Resolve-Path (Join-Path $PSScriptRoot "..\Output\Bin\WinVs2026$BuildType`64")).Path

function RunTest($name)
{
  Write-Host "`nRunning $name.`n" -ForegroundColor Yellow

  $executable = Join-Path $Path "$name.exe"

  if (-not (Test-Path $executable))
  {
    throw "Executable not found: $executable"
  }

  & $executable -nosave -all -nogui

  if ($LASTEXITCODE -ne 0)
  {
    Write-Host "`n$name failed`n" -ForegroundColor Yellow
    throw "Test failed."
  }

  Write-Host "`n$name succeeded.`n" -ForegroundColor Green
}

if (-not $SkipFoundationTest)
{
  RunTest "FoundationTest"
}

if (-not $SkipCoreTest)
{
  RunTest "CoreTest"
}

if (-not $SkipGraphicsFoundationTest)
{
  RunTest "GraphicsFoundationTest"
}

if (-not $SkipTextureTest)
{
  RunTest "TextureTest"
}

if (-not $SkipGraphicsCoreTest)
{
  RunTest "GraphicsCoreTest"
}

if (-not $SkipGameEngineTest)
{
  RunTest "GameEngineTest"
}

if (-not $SkipToolsFoundationTest)
{
  RunTest "ToolsFoundationTest"
}
