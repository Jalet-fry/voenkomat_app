# Script to automatically find and copy 32-bit libpq.dll
# Usage: .\install_32bit_libpq.ps1

Write-Host "=== Installing 32-bit libpq.dll ===" -ForegroundColor Green
Write-Host ""

# Target directory
$targetDir = "C:\QT_projects\voenkomat_app\debug"

# Check if target directory exists
if (-not (Test-Path $targetDir)) {
    Write-Host "ERROR: Target directory not found: $targetDir" -ForegroundColor Red
    Write-Host "Please make sure you run this script from the project root." -ForegroundColor Yellow
    exit 1
}

# Possible PostgreSQL 12/13 installation paths (32-bit)
$possiblePaths = @(
    "C:\Program Files (x86)\PostgreSQL\12\bin",
    "C:\Program Files (x86)\PostgreSQL\13\bin",
    "C:\Program Files\PostgreSQL\12\bin",
    "C:\Program Files\PostgreSQL\13\bin"
)

# Find PostgreSQL 12/13 installation
$foundPath = $null
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $libpqPath = Join-Path $path "libpq.dll"
        if (Test-Path $libpqPath) {
            $foundPath = $path
            Write-Host "Found PostgreSQL installation: $path" -ForegroundColor Green
            break
        }
    }
}

if (-not $foundPath) {
    Write-Host "ERROR: PostgreSQL 12 or 13 (32-bit) not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install PostgreSQL 12 or 13 (32-bit version):" -ForegroundColor Yellow
    Write-Host "1. Download from: https://www.postgresql.org/download/windows/" -ForegroundColor Cyan
    Write-Host "2. Choose 32-bit (x86) version" -ForegroundColor Cyan
    Write-Host "3. Install only 'Command Line Tools' (not the server)" -ForegroundColor Cyan
    Write-Host "4. Run this script again" -ForegroundColor Cyan
    exit 1
}

# Files to copy
$filesToCopy = @("libpq.dll", "libiconv-2.dll", "libintl-8.dll")

Write-Host ""
Write-Host "Copying files from: $foundPath" -ForegroundColor Yellow
Write-Host "To: $targetDir" -ForegroundColor Yellow
Write-Host ""

$copiedCount = 0
foreach ($file in $filesToCopy) {
    $sourceFile = Join-Path $foundPath $file
    $destFile = Join-Path $targetDir $file
    
    if (Test-Path $sourceFile) {
        try {
            Copy-Item $sourceFile $destFile -Force
            $fileSize = (Get-Item $sourceFile).Length
            Write-Host "  Copied: $file ($([math]::Round($fileSize/1KB, 2)) KB)" -ForegroundColor Green
            $copiedCount++
        } catch {
            Write-Host "  ERROR copying $file : $_" -ForegroundColor Red
        }
    } else {
        Write-Host "  Skipped: $file (not found, optional)" -ForegroundColor Yellow
    }
}

Write-Host ""
if ($copiedCount -gt 0) {
    Write-Host "=== Success! ===" -ForegroundColor Green
    Write-Host "Copied $copiedCount file(s) to $targetDir" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "You can now run your application!" -ForegroundColor Green
} else {
    Write-Host "ERROR: No files were copied!" -ForegroundColor Red
    exit 1
}

