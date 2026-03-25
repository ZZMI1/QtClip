param(
    [string]$BuildDir = "out",
    [string]$Config = "Release",
    [string]$OutputDir = "dist",
    [string]$PackageName = "QtClip-windows-x64",
    [switch]$SkipBuild = $false
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $repoRoot

if (-not $SkipBuild)
{
    Write-Host "[1/5] Configure project..."
    cmake -S . -B $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

    Write-Host "[2/5] Build project ($Config)..."
    cmake --build $BuildDir --config $Config --target qtclipsmoke
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }
}
else
{
    Write-Host "[1/5] Skip build step (using existing binaries)."
}

$releaseDir = Join-Path $repoRoot "$BuildDir\$Config"
$exePath = Join-Path $releaseDir "qtclipsmoke.exe"
if (!(Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

Write-Host "[3/5] Prepare package directory..."
$packageRoot = Join-Path $repoRoot $OutputDir
$packageDir = Join-Path $packageRoot $PackageName
$zipPath = Join-Path $packageRoot "$PackageName.zip"

if (Test-Path $packageDir) { Remove-Item $packageDir -Recurse -Force }
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

Write-Host "[4/5] Copy runtime files..."
Copy-Item (Join-Path $releaseDir "*") $packageDir -Recurse -Force

$readmePath = Join-Path $repoRoot "README_RELEASE.md"
if (Test-Path $readmePath) {
    Copy-Item $readmePath (Join-Path $packageDir "README_RELEASE.md") -Force
}

Write-Host "[5/5] Create zip archive..."
if (!(Test-Path $packageRoot)) { New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null }
Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath -Force

Write-Host "Package ready: $zipPath"
