@echo off
chcp 65001 > nul

:: --- КОНФИГУРАЦИЯ ---
set "QT_BIN=C:\Qt\Qt6.10\6.10.2\mingw_64\bin"
set "DLL_PATH=C:\QT_projects\voenkomat_app\server_cpp\dll"

:: Добавляем пути в PATH, чтобы экзешник нашел Qt-библиотеки
set "PATH=%QT_BIN%;%DLL_PATH%;%PATH%"

echo [INFO] Поиск исполняемого файла...

:: Проверяем конкретный путь, который есть у тебя на диске
set "APP_EXE=C:\QT_projects\voenkomat_app\server_cpp\build\Desktop_Qt_6_10_2_MinGW_64_bit-Release\release\voenkomat_server.exe"

if not exist "%APP_EXE%" (
    echo [ERROR] Файл не найден по пути:
    echo %APP_EXE%
    echo Пожалуйста, соберите проект в Qt Creator (Build -> Release)
    pause
    exit /b
)

echo [INFO] Запуск конвертера: %APP_EXE%
echo --------------------------------------------------

:: Запуск с флагом конвертации
"%APP_EXE%" --convert

echo --------------------------------------------------
echo [INFO] Конвертация завершена.
echo [INFO] Результаты ищи в папке: C:\QT_projects\voenkomat_app\nosql_db_cpp\

pause
