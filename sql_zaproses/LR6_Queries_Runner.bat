@echo off
REM Батник для последовательного выполнения всех SQL запросов из LR6
REM Использование: запустите LR6_Queries_Runner.bat

chcp 65001 >nul
setlocal enabledelayedexpansion

set "QUERIES_DIR=%~dp0LR6_Queries"
set "PAUSE_SECONDS=3"

REM Параметры подключения к БД (измените под ваши настройки)
set "DB_HOST=localhost"
set "DB_PORT=5432"
set "DB_NAME=voenkomat"
set "DB_USER=postgres"
REM Для пароля используйте переменную окружения PGPASSWORD
REM Пример: set "PGPASSWORD=your_password"

echo === Начало выполнения всех запросов LR6 ===
echo Папка с запросами: %QUERIES_DIR%
echo Пауза между запросами: %PAUSE_SECONDS% секунд
echo.

if not exist "%QUERIES_DIR%" (
    echo ОШИБКА: Папка %QUERIES_DIR% не найдена!
    pause
    exit /b 1
)

set /a SUCCESS_COUNT=0
set /a ERROR_COUNT=0

REM Получаем список всех SQL файлов и выполняем их по порядку
for %%F in ("%QUERIES_DIR%\*.sql") do (
    set "FILENAME=%%~nxF"
    set "QUERY_NUM=%%~nF"
    
    echo [!QUERY_NUM!] Выполняется запрос из файла: !FILENAME!
    
    REM Выполняем SQL через psql
    psql -h %DB_HOST% -p %DB_PORT% -U %DB_USER% -d %DB_NAME% -f "%%F"
    
    if !errorlevel! equ 0 (
        echo [!QUERY_NUM!] Успешно выполнено
        set /a SUCCESS_COUNT+=1
    ) else (
        echo [!QUERY_NUM!] ОШИБКА при выполнении
        set /a ERROR_COUNT+=1
    )
    
    echo Пауза %PAUSE_SECONDS% секунд...
    timeout /t %PAUSE_SECONDS% /nobreak >nul
    echo.
)

echo === Результаты выполнения ===
echo Успешно: %SUCCESS_COUNT%
echo Ошибок: %ERROR_COUNT%
echo.

pause

