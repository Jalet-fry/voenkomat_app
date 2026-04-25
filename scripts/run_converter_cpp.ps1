Write-Host "--- ЗАПУСК КОНВЕРТЕРА C++ (Qt/QDataStream) ---" -ForegroundColor Cyan

# Путь к папке проекта
$ProjectRoot = "C:\QT_projects\voenkomat_app"
$BuildRoot = Join-Path $ProjectRoot "server_cpp\build"

if (Test-Path $BuildRoot) {
    # Ищем папку, содержащую 'Release'
    $BuildDir = Get-ChildItem -Path $BuildRoot -Directory | Where-Object { $_.Name -like "*Release*" } | Select-Object -First 1

    if ($BuildDir) {
        $ExePath = Join-Path $BuildDir.FullName "release\voenkomat_server.exe"
        if (Test-Path $ExePath) {
            Write-Host "Запуск: $ExePath --convert"
            # Переходим в директорию экзешника, чтобы пути к конфигам считались верно
            Push-Location (Split-Path $ExePath)
            & $ExePath --convert
            Pop-Location
        } else {
            Write-Host "Ошибка: Экзешник не найден по пути $ExePath. Сначала собери проект в Qt Creator (Release)." -ForegroundColor Red
        }
    } else {
        Write-Host "Ошибка: Папка сборки Release не найдена внутри $BuildRoot. Проверьте настройки сборки в Qt Creator." -ForegroundColor Red
    }
} else {
    Write-Host "Ошибка: Папка $BuildRoot не существует. Сначала запустите сборку проекта в Qt Creator." -ForegroundColor Red
}
