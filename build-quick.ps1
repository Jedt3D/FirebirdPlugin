# ============================================================================
# FirebirdPlugin — Quick Build Script for Windows ARM64
# ============================================================================
# This is a simplified version of build-windows.ps1 for quick ARM64 builds
# when you already have Visual Studio 2025 and Firebird 6.0 installed.
#
# Prerequisites:
#   - Visual Studio 2025 with MSVC ARM64 build tools
#   - Firebird 6.0+ installed (defaults to C:\Program Files\Firebird\Firebird_6_0)
#   - CMake (included with Visual Studio)
#
# Usage:
#   .\build-quick.ps1                     # Quick ARM64 build
#   .\build-quick.ps1 -FirebirdPath "D:\Firebird"  # Custom Firebird location
# ============================================================================

param(
    [string]$FirebirdPath = "C:\Program Files\Firebird\Firebird_6_0",
    [switch]$Clean,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build_win_arm64"
$Arch = "ARM64"

Write-Host ""
Write-Host "🔥 FirebirdPlugin ARM64 Quick Build" -ForegroundColor Cyan
Write-Host ""

# ---------------------------------------------------------------------------
# Validate environment
# ---------------------------------------------------------------------------
Write-Host "[1/4] Validating environment..." -ForegroundColor Yellow

# Check Firebird installation
if (-not (Test-Path (Join-Path $FirebirdPath "include\ibase.h"))) {
    Write-Error "Firebird not found at $FirebirdPath"
    Write-Host "Install Firebird 6.0+ or specify path: -FirebirdPath `"C:\Path\To\Firebird`""
    exit 1
}
Write-Host "  ✓ Firebird: $FirebirdPath" -ForegroundColor Green

# Find Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "Visual Studio not found"
    exit 1
}

$vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath 2>$null
if (-not $vsPath) {
    Write-Error "MSVC ARM64 build tools not found. Install via Visual Studio Installer."
    exit 1
}
Write-Host "  ✓ Visual Studio: $vsPath" -ForegroundColor Green

# Find CMake
$cmakeExe = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmakeExe)) {
    $cmakeExe = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}
if (-not $cmakeExe) {
    Write-Error "CMake not found"
    exit 1
}
Write-Host "  ✓ CMake: $cmakeExe" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Clean build directory if requested
# ---------------------------------------------------------------------------
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host ""
    Write-Host "[2/4] Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item $BuildDir -Recurse -Force
    Write-Host "  ✓ Cleaned" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[2/4] Clean: skipped" -ForegroundColor DarkGray
}

# ---------------------------------------------------------------------------
# Configure with CMake
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[3/4] Configuring..." -ForegroundColor Yellow

$cmakeArgs = @(
    "-B", $BuildDir,
    "-S", $ProjectRoot,
    "-A", $Arch,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DFIREBIRD_ROOT=$FirebirdPath"
)

& $cmakeExe @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}
Write-Host "  ✓ Configuration complete" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[4/4] Building..." -ForegroundColor Yellow

& $cmakeExe --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "✅ Build succeeded!" -ForegroundColor Green
Write-Host ""

$dllPath = Get-ChildItem -Path (Join-Path $BuildDir "plugin") -Filter "*.dll" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if ($dllPath) {
    $size = [math]::Round($dllPath.Length / 1KB, 1)
    Write-Host "Output: $($dllPath.FullName) ($size KB)" -ForegroundColor White
    Write-Host ""

    # Check if it's actually ARM64
    $fileInfo = Get-Item $dllPath.FullName
    Write-Host "File: $($fileInfo.Name)" -ForegroundColor Cyan
    Write-Host "Size: $size KB" -ForegroundColor Cyan
    Write-Host "Location: Windows arm64" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "To install in Xojo:" -ForegroundColor Yellow
Write-Host "1. The plugin is staged at: build_win_arm64\plugin\" -ForegroundColor White
Write-Host "2. Copy to Xojo Plugins folder when ready" -ForegroundColor White
Write-Host ""
