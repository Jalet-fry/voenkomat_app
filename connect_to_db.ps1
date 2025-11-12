# Скрипт для подключения к базе данных voenkomat
# Использование: .\connect_to_db.ps1

$psqlPath = "C:\Program Files\PostgreSQL\17\bin\psql.exe"

# Проверка существования psql
if (-not (Test-Path $psqlPath)) {
    Write-Host "ОШИБКА: psql.exe не найден по пути: $psqlPath" -ForegroundColor Red
    Write-Host "Проверьте путь к PostgreSQL" -ForegroundColor Yellow
    exit 1
}

Write-Host "=== Подключение к базе данных voenkomat ===" -ForegroundColor Green
Write-Host ""

# Параметры подключения
$host = "localhost"
$port = "5432"
$database = "voenkomat"
$username = "postgres"

# Запрос пароля
Write-Host "Введите пароль для пользователя $username:" -ForegroundColor Yellow
$securePassword = Read-Host -AsSecureString
$password = [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePassword))

# Установить переменную окружения для пароля
$env:PGPASSWORD = $password

Write-Host ""
Write-Host "Подключение..." -ForegroundColor Cyan
Write-Host ""

# Подключиться к базе данных
& $psqlPath -h $host -p $port -U $username -d $database

# Очистить пароль из памяти
$env:PGPASSWORD = $null
$password = $null

