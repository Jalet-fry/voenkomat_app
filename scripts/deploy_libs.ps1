$ProjectRoot = "C:\QT_projects\voenkomat_app"
$ReleaseDir = Get-ChildItem -Path "$ProjectRoot\server_cpp\build" -Directory | Where-Object { $_.Name -like "*Release*" } | Select-Object -First 1
$TargetDir = Join-Path $ReleaseDir.FullName "release"

Write-Host "Целевая папка: $TargetDir" -ForegroundColor Cyan

# 1. Копируем всё из нашей папки /dll/ (там libpq, libdb и др.)
$DllSource = Join-Path $ProjectRoot "dll"
if (Test-Path $DllSource) {
    Write-Host "Копирование базовых DLL..."
    Copy-Item -Path "$DllSource\*" -Destination $TargetDir -Force
}

# 2. Используем windeployqt (стандартный инструмент Qt) для копирования библиотек Qt
$QtBin = "C:\Qt\Qt6.10\6.10.2\mingw_64\bin"
$WinDeployQt = Join-Path $QtBin "windeployqt.exe"

if (Test-Path $WinDeployQt) {
    Write-Host "Запуск windeployqt для сборки зависимостей Qt..."
    & $WinDeployQt --no-translations --no-compiler-runtime --no-opengl-sw --no-svg "$TargetDir\voenkomat_server.exe"
} else {
    Write-Host "Ошибка: windeployqt не найден в $QtBin" -ForegroundColor Red
}

Write-Host "Готово! Попробуйте запустить voenkomat_server.exe из папки release снова." -ForegroundColor Green
