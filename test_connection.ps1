# Скрипт для тестирования подключения к БД
# ВАЖНО: Замените "YOUR_PASSWORD" на ваш реальный пароль!

Write-Host "=== Тест подключения к базе данных voenkomat ===" -ForegroundColor Green
Write-Host ""

# ============================================
# ВАЖНО: ЗАМЕНИТЕ "YOUR_PASSWORD" НА ВАШ РЕАЛЬНЫЙ ПАРОЛЬ!
# ============================================
$env:PGPASSWORD = "YOUR_PASSWORD"

$psqlPath = "C:\Program Files\PostgreSQL\17\bin\psql.exe"

Write-Host "Попытка подключения..." -ForegroundColor Cyan
Write-Host ""

# Попытка подключения и выполнение простого запроса
$result = & $psqlPath -h localhost -p 5432 -U postgres -d voenkomat -c "SELECT current_database(), current_user, version();" 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ УСПЕШНО ПОДКЛЮЧЕНО!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Результат запроса:" -ForegroundColor Cyan
    Write-Host $result
    Write-Host ""
    Write-Host "Параметры подключения:" -ForegroundColor Yellow
    Write-Host "  Хост: localhost"
    Write-Host "  Порт: 5432"
    Write-Host "  База данных: voenkomat"
    Write-Host "  Пользователь: postgres"
    Write-Host "  Пароль: [установлен]" -ForegroundColor Gray
} else {
    Write-Host "✗ ОШИБКА ПОДКЛЮЧЕНИЯ" -ForegroundColor Red
    Write-Host ""
    Write-Host "Возможные причины:" -ForegroundColor Yellow
    Write-Host "  1. Неправильный пароль"
    Write-Host "  2. База данных не запущена"
    Write-Host "  3. Неправильные параметры подключения"
    Write-Host ""
    Write-Host "Детали ошибки:" -ForegroundColor Red
    Write-Host $result
}

# Очистить пароль
$env:PGPASSWORD = $null

Write-Host ""
Write-Host "Нажмите любую клавишу для выхода..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

