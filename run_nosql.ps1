$QtBin = "C:\Qt\Qt6.10\6.10.2\mingw_64\bin"
$DllPath = "C:\QT_projects\voenkomat_app\server_cpp\dll"
$BdbDllPath = "C:\QT_projects\voenkomat_app\server_cpp\bdb\bin"
$ExePath = "C:\QT_projects\voenkomat_app\server_cpp\build\Desktop_Qt_6_10_2_MinGW_64_bit-Release\release\voenkomat_server.exe"

# 1. Проверка наличия файла
if (-not (Test-Path $ExePath)) {
    Write-Host "[ERROR] Файл не найден: $ExePath" -ForegroundColor Red
    Write-Host "Пожалуйста, соберите проект в Qt Creator (Release)."
    exit
}

# 2. Настройка окружения (PATH)
# Добавляем путь к BDB DLL, чтобы экзешник её увидел
$env:PATH = "$QtBin;$DllPath;$BdbDllPath;" + $env:PATH

Write-Host "[INFO] Запуск NoSQL конвертера (C++ Berkeley DB)..." -ForegroundColor Cyan
Write-Host "[INFO] EXE: $ExePath"

# 3. Запуск
& $ExePath --convert

Write-Host "`n[INFO] Конвертация завершена." -ForegroundColor Green
Write-Host "[INFO] Результаты в папке: C:\QT_projects\voenkomat_app\nosql_db_cpp\"
