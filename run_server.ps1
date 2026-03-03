Set-Location -Path "$PSScriptRoot\server_python"
Write-Host "Запуск сервера военкомата..." -ForegroundColor Green
python main.py
if ($LASTEXITCODE -ne 0) {
    Write-Host "Ошибка при запуске сервера!" -ForegroundColor Red
    Read-Host "Нажмите Enter, чтобы выйти..."
}
