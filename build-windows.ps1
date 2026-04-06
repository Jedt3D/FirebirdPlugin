# ============================================================================
# FirebirdPlugin — Windows Build Script
# ============================================================================
# Prerequisites:
#   - Visual Studio 2022+ with "Desktop development with C++" workload
#   - For ARM64: add "MSVC ARM64 build tools" component (Individual Components)
#   - CMake (ships with Visual Studio, or install separately)
#
# Usage:
#   .\build-windows.ps1                    # build x64 (default)
#   .\build-windows.ps1 -Arch arm64        # build ARM64
#   .\build-windows.ps1 -Arch x64 -Clean   # clean rebuild
#   .\build-windows.ps1 -SkipFirebird      # skip Firebird download (already installed)
#   .\build-windows.ps1 -FirebirdRoot "D:\Firebird"   # use custom Firebird path
#
# The script will:
#   1. Locate Visual Studio 2022
#   2. Download & extract Firebird client (if needed)
#   3. Configure with CMake
#   4. Build the plugin DLL
#   5. Stage the .xojo_plugin structure
# ============================================================================

param(
    [ValidateSet("x64", "arm64")]
    [string]$Arch = "x64",

    [string]$FirebirdRoot = "",

    [switch]$SkipFirebird,

    [switch]$Clean,

    [string]$BuildType = "Release",

    [string]$FirebirdVersion = "6.0.0",  # Updated for Firebird 6.0 Beta

    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
$PluginName = "FirebirdPlugin"
$ProjectRoot = $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build_win_$Arch"
$DepsDir = Join-Path $ProjectRoot "deps"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  FirebirdPlugin Windows Build" -ForegroundColor Cyan
Write-Host "  Architecture: $Arch" -ForegroundColor Cyan
Write-Host "  Config:       $BuildType" -ForegroundColor Cyan
Write-Host "  Firebird:     $FirebirdVersion" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ARM64 notice
if ($Arch -eq "arm64") {
    Write-Host "  NOTE: Building native ARM64 plugin with x64 Firebird client" -ForegroundColor Yellow
    Write-Host "  (Firebird client will run under WOW64 emulation)" -ForegroundColor Yellow
    Write-Host ""
}

# ---------------------------------------------------------------------------
# Step 1: Find Visual Studio 2022
# ---------------------------------------------------------------------------
Write-Host "[1/5] Locating Visual Studio..." -ForegroundColor Yellow

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "vswhere.exe not found. Is Visual Studio installed?"
    exit 1
}

$vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
if (-not $vsPath) {
    Write-Error "Visual Studio with C++ tools not found. Install 'Desktop development with C++' workload."
    exit 1
}

Write-Host "  Found: $vsPath" -ForegroundColor Green

# Check ARM64 tools if needed
if ($Arch -eq "arm64") {
    $arm64Tools = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath 2>$null
    if (-not $arm64Tools) {
        Write-Host ""
        Write-Host "  WARNING: ARM64 build tools not found!" -ForegroundColor Red
        Write-Host "  Open Visual Studio Installer and add:" -ForegroundColor Red
        Write-Host "    'MSVC ARM64 build tools' under Individual Components." -ForegroundColor Red
        Write-Host ""
        Write-Host "  You can still build for x64, but ARM64 requires these tools." -ForegroundColor Yellow
        exit 1
    }
    Write-Host "  ARM64 tools: OK" -ForegroundColor Green
}

# Find CMake (prefer VS bundled, fall back to system)
$cmakeExe = $null
$vsCMake = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (Test-Path $vsCMake) {
    $cmakeExe = $vsCMake
} else {
    $cmakeExe = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}
if (-not $cmakeExe) {
    Write-Error "CMake not found. Install CMake or ensure the VS CMake component is installed."
    exit 1
}
Write-Host "  CMake: $cmakeExe" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Step 2: Download Firebird client
# ---------------------------------------------------------------------------
if (-not $SkipFirebird -and -not $FirebirdRoot) {
    Write-Host ""
    Write-Host "[2/5] Setting up Firebird $FirebirdVersion client..." -ForegroundColor Yellow

    # For ARM64 builds, we use x64 Firebird client (works under emulation)
    $fbArch = if ($Arch -eq "arm64") { "x64" } else { $Arch }
    $fbDir = Join-Path $DepsDir "firebird_$fbArch"

    $fbDir = Join-Path $DepsDir "firebird_$($Arch)"
    if (Test-Path (Join-Path $fbDir "include\ibase.h")) {
        Write-Host "  Already downloaded at: $fbDir" -ForegroundColor Green
        $FirebirdRoot = $fbDir
    } else {
        New-Item -ItemType Directory -Path $DepsDir -Force | Out-Null

        $dlPath = Join-Path $DepsDir "firebird.zip"

        # Firebird 6.0 download URLs - use x64 for both architectures (ARM64 uses x64 client)
        if ($FirebirdVersion -eq "6.0.0") {
            # Firebird 6.0 Beta URLs
            $fbUrl = "https://github.com/FirebirdSQL/firebird/releases/download/v6.0.0-beta1/Firebird-6.0.0.1880-0-windows-x64.zip"
        } else {
            # Fallback to 5.x stable
            $fbUrl = "https://github.com/FirebirdSQL/firebird/releases/download/v${FirebirdVersion}/Firebird-${FirebirdVersion}-windows-x64.zip"
        }

        Write-Host "  Downloading: $fbUrl"
        try {
            Invoke-WebRequest -Uri $fbUrl -OutFile $dlPath -UseBasicParsing
        } catch {
            Write-Host "  Download failed, trying alternative source..." -ForegroundColor DarkYellow
            # Alternative Firebird download locations
            if ($FirebirdVersion -eq "6.0.0") {
                $fbUrl = "https://firebirdsql.org/en/server-packages/Firebird-6.0.0.1880-0-windows-x64.zip"
            } else {
                $fbUrl = "https://firebirdsql.org/en/server-packages/Firebird-${FirebirdVersion}-windows-x64.zip"
            }
            Invoke-WebRequest -Uri $fbUrl -OutFile $dlPath -UseBasicParsing
        }

        Write-Host "  Extracting..."
        $extractDir = Join-Path $DepsDir "firebird_extract"
        if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
        Expand-Archive -Path $dlPath -DestinationPath $extractDir -Force

        # Firebird ZIP may have a nested directory structure
        $nested = Get-ChildItem $extractDir -Directory | Select-Object -First 1
        if ($nested -and (Test-Path (Join-Path $nested.FullName "include"))) {
            $srcDir = $nested.FullName
        } else {
            # Check for deeper nesting (common in Firebird packages)
            $deepNested = Get-ChildItem $extractDir -Directory -Recurse | Where-Object { Test-Path (Join-Path $_.FullName "include") } | Select-Object -First 1
            if ($deepNested) {
                $srcDir = $deepNested.FullName
            } else {
                $srcDir = $extractDir
            }
        }

        # Copy to architecture-specific directory (x64 for both x64 and ARM64 builds)
        if (Test-Path $fbDir) { Remove-Item $fbDir -Recurse -Force }
        Copy-Item -Path $srcDir -Destination $fbDir -Recurse

        if ($Verbose) {
            Write-Host "  Extracted from: $srcDir" -ForegroundColor DarkGray
            Write-Host "  Contents of fbDir:" -ForegroundColor DarkGray
            Get-ChildItem $fbDir | ForEach-Object { Write-Host "    $($_.Name)" -ForegroundColor DarkGray }
        }

        # Clean up
        Remove-Item $dlPath -Force -ErrorAction SilentlyContinue
        Remove-Item $extractDir -Recurse -Force -ErrorAction SilentlyContinue

        # Verify
        if (-not (Test-Path (Join-Path $fbDir "include\ibase.h"))) {
            Write-Error "Firebird extraction failed — include\ibase.h not found in $fbDir"
            exit 1
        }

        $FirebirdRoot = $fbDir
        Write-Host "  Installed to: $FirebirdRoot" -ForegroundColor Green
    }
} elseif ($SkipFirebird -and -not $FirebirdRoot) {
    # Try auto-detect
    $searchPaths = @(
        "C:\Program Files\Firebird\Firebird_6_0",
        "C:\Program Files\Firebird\Firebird_5_0",
        "C:\Program Files\Firebird\Firebird_4_0",
        "C:\Firebird",
        (Join-Path $DepsDir "firebird_$Arch")
    )
    foreach ($p in $searchPaths) {
        if (Test-Path (Join-Path $p "include\ibase.h")) {
            $FirebirdRoot = $p
            break
        }
    }
    if (-not $FirebirdRoot) {
        Write-Error "Firebird not found. Run without -SkipFirebird or specify -FirebirdRoot."
        exit 1
    }
    Write-Host ""
    Write-Host "[2/5] Using existing Firebird at: $FirebirdRoot" -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "[2/5] Using Firebird at: $FirebirdRoot" -ForegroundColor Yellow
    if (-not (Test-Path (Join-Path $FirebirdRoot "include\ibase.h"))) {
        Write-Error "ibase.h not found in $FirebirdRoot\include\"
        exit 1
    }
}

# ---------------------------------------------------------------------------
# Step 3: Clean (if requested)
# ---------------------------------------------------------------------------
# Clean stale CMake cache if it exists (avoids generator mismatch errors)
$cmakeCache = Join-Path $BuildDir "CMakeCache.txt"
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host ""
    Write-Host "[3/5] Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item $BuildDir -Recurse -Force
    Write-Host "  Removed: $BuildDir" -ForegroundColor Green
} elseif (Test-Path $cmakeCache) {
    # Check if the cached generator matches; if not, wipe and reconfigure
    $cachedGen = Select-String -Path $cmakeCache -Pattern "^CMAKE_GENERATOR:" -ErrorAction SilentlyContinue
    if ($cachedGen) {
        Write-Host ""
        Write-Host "[3/5] Existing build cache found (will reconfigure if needed)" -ForegroundColor DarkGray
    }
} else {
    Write-Host ""
    Write-Host "[3/5] Clean: skipped" -ForegroundColor DarkGray
}

# ---------------------------------------------------------------------------
# Step 4: CMake configure
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[4/5] Configuring with CMake..." -ForegroundColor Yellow

# Map arch to CMake generator platform
$cmakeArch = switch ($Arch) {
    "x64"   { "x64" }
    "arm64" { "ARM64" }
}

# Let CMake auto-detect the VS generator (avoids hardcoding version names)
$cmakeArgs = @(
    "-B", $BuildDir,
    "-S", $ProjectRoot,
    "-A", $cmakeArch,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DFIREBIRD_ROOT=$FirebirdRoot"
)

Write-Host "  > cmake $($cmakeArgs -join ' ')" -ForegroundColor DarkGray
& $cmakeExe @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configure failed (exit code $LASTEXITCODE)"
    exit 1
}
Write-Host "  Configure: OK" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Step 5: Build
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "[5/5] Building ($BuildType)..." -ForegroundColor Yellow

& $cmakeExe --build $BuildDir --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed (exit code $LASTEXITCODE)"
    exit 1
}

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
$pluginDir = Join-Path $BuildDir "plugin"
$dllPath = Get-ChildItem -Path $pluginDir -Filter "*.dll" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  BUILD SUCCEEDED" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
if ($dllPath) {
    $size = [math]::Round($dllPath.Length / 1KB, 1)
    Write-Host "  Output: $($dllPath.FullName) ($size KB)" -ForegroundColor White
}
Write-Host "  Plugin staged at: $pluginDir" -ForegroundColor White
Write-Host ""
Write-Host "  To package as .xojo_plugin, run from the plugin dir:" -ForegroundColor DarkGray
Write-Host "    Compress-Archive -Path '$pluginDir\*' -DestinationPath '$PluginName.xojo_plugin'" -ForegroundColor DarkGray
Write-Host ""
