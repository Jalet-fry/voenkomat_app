# Script to copy new libpq.dll to debug folder
# Usage: .\copy_new_libpq.ps1 [path_to_downloaded_libpq.dll]

Write-Host "=== Copying new libpq.dll to debug folder ===" -ForegroundColor Green
Write-Host ""

$targetDir = "C:\QT_projects\voenkomat_app\debug"
$targetFile = Join-Path $targetDir "libpq.dll"

# Check if target directory exists
if (-not (Test-Path $targetDir)) {
    Write-Host "ERROR: Target directory not found: $targetDir" -ForegroundColor Red
    exit 1
}

# If source file is provided as argument
if ($args.Count -gt 0) {
    $sourceFile = $args[0]
} else {
    # Try to find the file in common locations
    $possibleLocations = @(
        "$env:USERPROFILE\Downloads\libpq.dll",
        "$env:USERPROFILE\Desktop\libpq.dll",
        ".\libpq.dll"
    )
    
    $sourceFile = $null
    foreach ($location in $possibleLocations) {
        if (Test-Path $location) {
            $fileInfo = Get-Item $location
            # Check if it's the new file (should be around 722 KB)
            if ($fileInfo.Length -gt 700000) {
                $sourceFile = $location
                Write-Host "Found new libpq.dll at: $location" -ForegroundColor Cyan
                Write-Host "  Size: $($fileInfo.Length) bytes ($([math]::Round($fileInfo.Length/1KB, 2)) KB)" -ForegroundColor Cyan
                break
            }
        }
    }
    
    if (-not $sourceFile) {
        Write-Host "ERROR: New libpq.dll not found!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Please provide the path to the downloaded libpq.dll:" -ForegroundColor Yellow
        Write-Host "  .\copy_new_libpq.ps1 \"C:\path\to\libpq.dll\"" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Or place libpq.dll in one of these locations:" -ForegroundColor Yellow
        foreach ($loc in $possibleLocations) {
            Write-Host "  - $loc" -ForegroundColor Yellow
        }
        exit 1
    }
}

# Verify source file
if (-not (Test-Path $sourceFile)) {
    Write-Host "ERROR: Source file not found: $sourceFile" -ForegroundColor Red
    exit 1
}

$sourceInfo = Get-Item $sourceFile
Write-Host "Source file: $sourceFile" -ForegroundColor Cyan
Write-Host "  Size: $($sourceInfo.Length) bytes ($([math]::Round($sourceInfo.Length/1KB, 2)) KB)" -ForegroundColor Cyan

# Check if it's the right size (should be around 722 KB for PostgreSQL 16.3)
if ($sourceInfo.Length -lt 700000) {
    Write-Host "WARNING: File size seems too small. Expected ~722 KB for PostgreSQL 16.3 (32-bit)" -ForegroundColor Yellow
    $response = Read-Host "Continue anyway? (y/n)"
    if ($response -ne "y") {
        exit 1
    }
}

# Backup old file if it exists
if (Test-Path $targetFile) {
    $oldFile = Join-Path $targetDir "libpq.dll.old"
    $oldInfo = Get-Item $targetFile
    Write-Host ""
    Write-Host "Backing up old libpq.dll..." -ForegroundColor Yellow
    Write-Host "  Old size: $($oldInfo.Length) bytes ($([math]::Round($oldInfo.Length/1KB, 2)) KB)" -ForegroundColor Yellow
    
    if (Test-Path $oldFile) {
        Remove-Item $oldFile -Force
    }
    Copy-Item $targetFile $oldFile -Force
    Write-Host "  Backed up to: libpq.dll.old" -ForegroundColor Green
}

# Copy new file
Write-Host ""
Write-Host "Copying new libpq.dll..." -ForegroundColor Yellow
try {
    Copy-Item $sourceFile $targetFile -Force
    Write-Host "  SUCCESS! New libpq.dll copied to: $targetFile" -ForegroundColor Green
    
    # Verify
    $newInfo = Get-Item $targetFile
    Write-Host ""
    Write-Host "Verification:" -ForegroundColor Cyan
    Write-Host "  Size: $($newInfo.Length) bytes ($([math]::Round($newInfo.Length/1KB, 2)) KB)" -ForegroundColor Cyan
    Write-Host "  Modified: $($newInfo.LastWriteTime)" -ForegroundColor Cyan
    
    if ($newInfo.Length -gt 700000) {
        Write-Host ""
        Write-Host "✓ File looks correct! You can now test your application." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "⚠ WARNING: File size seems incorrect. Expected ~722 KB." -ForegroundColor Yellow
    }
} catch {
    Write-Host "ERROR: Failed to copy file: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green

