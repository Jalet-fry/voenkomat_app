@echo off
set "QT_PATH=C:\Qt\Qt6.10\6.10.2\mingw_64\bin"
set "APP_PATH=C:\QT_projects\voenkomat_app\server_cpp\build\Desktop_Qt_6_10_2_MinGW_64_bit-Release\release"
set "DLL_PATH=C:\QT_projects\voenkomat_app\dll"

:: Настройка окружения
set "PATH=%QT_PATH%;%DLL_PATH%;%PATH%"

echo [INFO] Запуск NoSQL конвертера...
if exist "%APP_PATH%\voenkomat_server.exe" (
    "%APP_PATH%\voenkomat_server.exe" --convert
) else (
    echo [ERROR] Файл voenkomat_server.exe не найден!
    echo Проверьте путь: %APP_PATH%
)

pause
