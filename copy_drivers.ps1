# Script to copy Qt SQL drivers to application folder
# Usage: .\copy_drivers.ps1

Write-Host "=== Copying Qt SQL Drivers ===" -ForegroundColor Green
Write-Host ""

# Paths
$qtPluginsPath = "C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers"
$pgBinPath = "C:\Program Files\PostgreSQL\17\bin"

# Check if Qt plugins path exists
if (-not (Test-Path $qtPluginsPath)) {
    Write-Host "ERROR: Qt drivers path not found: $qtPluginsPath" -ForegroundColor Red
    Write-Host "Please check Qt installation path." -ForegroundColor Yellow
    exit 1
}

# Check if PostgreSQL path exists
if (-not (Test-Path $pgBinPath)) {
    Write-Host "WARNING: PostgreSQL path not found: $pgBinPath" -ForegroundColor Yellow
    Write-Host "Please find libpq.dll manually." -ForegroundColor Yellow
}

# Function to copy drivers
function Copy-Drivers {
    param(
        [string]$TargetDir,
        [string]$DriverName
    )
    
    $targetPluginsDir = Join-Path $TargetDir "plugins\sqldrivers"
    
    # Create directory structure
    New-Item -ItemType Directory -Force -Path $targetPluginsDir | Out-Null
    Write-Host "Created folder: $targetPluginsDir" -ForegroundColor Cyan
    
    # Copy driver
    $sourceDriver = Join-Path $qtPluginsPath $DriverName
    $destDriver = Join-Path $targetPluginsDir $DriverName
    
    if (Test-Path $sourceDriver) {
        Copy-Item $sourceDriver $destDriver -Force
        Write-Host "  Copied driver: $DriverName" -ForegroundColor Green
    } else {
        Write-Host "  Driver not found: $sourceDriver" -ForegroundColor Red
    }
    
    # Copy PostgreSQL dependencies
    $dlls = @("libpq.dll", "libintl-8.dll", "libiconv-2.dll")
    foreach ($dll in $dlls) {
        $sourceDll = Join-Path $pgBinPath $dll
        if (Test-Path $sourceDll) {
            $destDll = Join-Path $TargetDir $dll
            Copy-Item $sourceDll $destDll -Force
            Write-Host "  Copied dependency: $dll" -ForegroundColor Green
        }
    }
}

# Copy for Debug build
Write-Host "Copying for Debug build..." -ForegroundColor Yellow
if (Test-Path "debug") {
    Copy-Drivers "debug" "qsqlpsqld.dll"
} else {
    Write-Host "  Skipped: debug folder not found" -ForegroundColor Yellow
}

Write-Host ""

# Copy for Release build
Write-Host "Copying for Release build..." -ForegroundColor Yellow
if (Test-Path "release") {
    Copy-Drivers "release" "qsqlpsql.dll"
} else {
    Write-Host "  Skipped: release folder not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "=== Done! ===" -ForegroundColor Green
Write-Host "Drivers copied. You can now run the application." -ForegroundColor Cyan
Write-Host ""
