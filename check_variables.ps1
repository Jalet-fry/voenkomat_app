# Скрипт для проверки переменных окружения PostgreSQL

Write-Host "=== Проверка переменных окружения PostgreSQL ===" -ForegroundColor Green
Write-Host ""

# Проверка PGPASSWORD
Write-Host "1. Переменная PGPASSWORD:" -ForegroundColor Yellow
if ($env:PGPASSWORD) {
    Write-Host "   Значение: $($env:PGPASSWORD)" -ForegroundColor Green
    Write-Host "   Длина: $($env:PGPASSWORD.Length) символов" -ForegroundColor Gray
} else {
    Write-Host "   НЕ УСТАНОВЛЕНА" -ForegroundColor Red
}

Write-Host ""

# Проверка других переменных PostgreSQL
Write-Host "2. Все переменные окружения, связанные с PostgreSQL:" -ForegroundColor Yellow
$pgVars = Get-ChildItem Env: | Where-Object { $_.Name -like "*PG*" -or $_.Name -like "*POSTGRES*" }
if ($pgVars) {
    foreach ($var in $pgVars) {
        Write-Host "   $($var.Name) = $($var.Value)" -ForegroundColor Cyan
    }
} else {
    Write-Host "   Не найдено переменных, связанных с PostgreSQL" -ForegroundColor Gray
}

Write-Host ""

# Проверка пути к psql
Write-Host "3. Путь к psql.exe:" -ForegroundColor Yellow
$psqlPath = "C:\Program Files\PostgreSQL\17\bin\psql.exe"
if (Test-Path $psqlPath) {
    Write-Host "   Найден: $psqlPath" -ForegroundColor Green
} else {
    Write-Host "   НЕ НАЙДЕН" -ForegroundColor Red
}

Write-Host ""
Write-Host "=== Инструкция ===" -ForegroundColor Green
Write-Host ""
Write-Host "Чтобы установить пароль, выполните:" -ForegroundColor Yellow
Write-Host '   $env:PGPASSWORD = "ваш_пароль"' -ForegroundColor Cyan
Write-Host ""
Write-Host "Чтобы вывести значение пароля:" -ForegroundColor Yellow
Write-Host '   $env:PGPASSWORD' -ForegroundColor Cyan
Write-Host ""
Write-Host "Чтобы очистить пароль:" -ForegroundColor Yellow
Write-Host "   `$env:PGPASSWORD = `$null" -ForegroundColor Cyan
Write-Host ""

