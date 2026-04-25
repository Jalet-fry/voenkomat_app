Write-Host "--- УСТАНОВКА ЗАВИСИМОСТЕЙ PYTHON ---" -ForegroundColor Cyan
$PythonExe = "C:\Python310\python.exe"

if (Test-Path $PythonExe) {
    & $PythonExe -m pip install --upgrade pip
    & $PythonExe -m pip install -r C:\QT_projects\voenkomat_app\requirements.txt
} else {
    Write-Host "Ошибка: Python не найден по пути $PythonExe" -ForegroundColor Red
}
