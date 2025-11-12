# Script to check driver dependencies
# Usage: .\check_driver_dependencies.ps1

Write-Host "=== Checking Qt SQL Driver Dependencies ===" -ForegroundColor Green
Write-Host ""

$driverPath = "C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers\qsqlpsqld.dll"

if (-not (Test-Path $driverPath)) {
    Write-Host "ERROR: Driver not found: $driverPath" -ForegroundColor Red
    exit 1
}

Write-Host "Driver found: $driverPath" -ForegroundColor Green
Write-Host ""

# Check dependencies using dumpbin (if available) or just list Qt DLLs
Write-Host "Checking for Qt dependencies in application folder..." -ForegroundColor Yellow

$appDir = "C:\QT_projects\voenkomat_app\debug"
$qtDlls = @("Qt5Sqld.dll", "Qt5Cored.dll", "Qt5Guid.dll", "Qt5Widgetsd.dll")

Write-Host "Required Qt DLLs:" -ForegroundColor Cyan
foreach ($dll in $qtDlls) {
    $dllPath = Join-Path $appDir $dll
    if (Test-Path $dllPath) {
        Write-Host "  $dll: Found" -ForegroundColor Green
    } else {
        Write-Host "  $dll: NOT FOUND" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "PostgreSQL dependencies:" -ForegroundColor Cyan
$pgDlls = @("libpq.dll", "libiconv-2.dll", "libintl-8.dll")
foreach ($dll in $pgDlls) {
    $dllPath = Join-Path $appDir $dll
    if (Test-Path $dllPath) {
        Write-Host "  $dll: Found" -ForegroundColor Green
    } else {
        Write-Host "  $dll: NOT FOUND" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Green
Write-Host "If Qt DLLs are missing, they should be in:" -ForegroundColor Yellow
Write-Host "  C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\" -ForegroundColor Cyan
Write-Host ""

