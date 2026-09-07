Write-Host "--- ЗАПУСК PYTHON СЕРВЕРА (FastAPI) ---" -ForegroundColor Cyan

$PythonDir = "C:\QT_projects\voenkomat_app\server_python"
$PythonExe = "C:\Python310\python.exe"

if (Test-Path "$PythonDir\main.py") {
    Push-Location $PythonDir

    Write-Host "Используем путь: $PythonExe"
    # Запускаем через uvicorn или напрямую если uvicorn прописан в main
    & $PythonExe main.py

    Pop-Location
} else {
    Write-Host "Ошибка: Файл main.py не найден в $PythonDir" -ForegroundColor Red
}
