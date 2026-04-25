Write-Host "--- ЗАПУСК КОНВЕРТЕРА PYTHON (dbm/BerkeleyDB) ---" -ForegroundColor Cyan

$PythonDir = "C:\QT_projects\voenkomat_app\server_python"
$PythonExe = "C:\Python310\python.exe"

if (Test-Path "$PythonDir\converter.py") {
    Push-Location $PythonDir

    Write-Host "Используем путь: $PythonExe"
    & $PythonExe converter.py

    if ($LASTEXITCODE -ne 0) {
        Write-Host "`nОшибка при выполнении скрипта. Проверьте установку python и библиотеки psycopg2." -ForegroundColor Red
        Write-Host "Попробуйте выполнить: & '$PythonExe' -m pip install psycopg2-binary" -ForegroundColor Gray
    }

    Pop-Location
} else {
    Write-Host "Ошибка: Файл converter.py не найден в $PythonDir" -ForegroundColor Red
}
