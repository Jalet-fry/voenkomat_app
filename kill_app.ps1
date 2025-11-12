# Скрипт для завершения процесса voenkomat_app.exe перед сборкой
# Использование: .\kill_app.ps1

Write-Host "=== Завершение процесса voenkomat_app.exe ===" -ForegroundColor Green
Write-Host ""

$processName = "voenkomat_app"
$processes = Get-Process -Name $processName -ErrorAction SilentlyContinue

if ($processes) {
    Write-Host "Найдено процессов: $($processes.Count)" -ForegroundColor Yellow
    foreach ($proc in $processes) {
        Write-Host "  Завершение процесса ID: $($proc.Id) (PID: $($proc.Id))" -ForegroundColor Cyan
        try {
            Stop-Process -Id $proc.Id -Force -ErrorAction Stop
            Write-Host "  ✓ Процесс завершен" -ForegroundColor Green
        } catch {
            Write-Host "  ✗ Ошибка при завершении: $_" -ForegroundColor Red
        }
    }
    Write-Host ""
    Write-Host "✓ Все процессы завершены. Теперь можно выполнить сборку." -ForegroundColor Green
} else {
    Write-Host "Процесс $processName не найден. Можно выполнять сборку." -ForegroundColor Green
}

Write-Host ""

