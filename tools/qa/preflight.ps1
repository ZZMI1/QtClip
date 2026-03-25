param(
    [string]$BuildDir = "out",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "[1/3] Build qtclipsmoke..."
cmake --build $BuildDir --config $Config --target qtclipsmoke
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

Write-Host "[2/3] Verify AI config..."
& "$BuildDir\$Config\qtclipsmoke.exe" --test-ai-config
if ($LASTEXITCODE -ne 0) { throw "AI config verification failed." }

Write-Host "[3/3] Run smoke workflow..."
& "$BuildDir\$Config\qtclipsmoke.exe" --smoke
if ($LASTEXITCODE -ne 0) { throw "Smoke workflow failed." }

Write-Host "Preflight passed."
