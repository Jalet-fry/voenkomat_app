# Simple script to show PGPASSWORD variable

Write-Host "Current PGPASSWORD value:" -ForegroundColor Green
if ($env:PGPASSWORD) {
    Write-Host $env:PGPASSWORD -ForegroundColor Yellow
} else {
    Write-Host "NOT SET" -ForegroundColor Red
}

