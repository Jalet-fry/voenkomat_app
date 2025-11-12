# Скрипт для установки переменной окружения PGPASSWORD
# Использование: .\set_password.ps1
# После выполнения этого скрипта можно запускать приложение Qt

Write-Host "=== Установка переменной окружения PGPASSWORD ===" -ForegroundColor Green
Write-Host ""

# Запрос пароля
Write-Host "Введите пароль для пользователя postgres:" -ForegroundColor Yellow
$securePassword = Read-Host -AsSecureString
$password = [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($securePassword))

# Установить переменную окружения для текущей сессии
$env:PGPASSWORD = $password

Write-Host ""
Write-Host "✓ Переменная окружения PGPASSWORD установлена для текущей сессии PowerShell" -ForegroundColor Green
Write-Host ""
Write-Host "Теперь вы можете запустить приложение Qt из этой же сессии PowerShell." -ForegroundColor Cyan
Write-Host ""
Write-Host "ВАЖНО: Переменная окружения будет действовать только в этой сессии PowerShell." -ForegroundColor Yellow
Write-Host "Если вы закроете окно PowerShell, переменная будет потеряна." -ForegroundColor Yellow
Write-Host ""
Write-Host "Для постоянной установки (для всех сессий) используйте:" -ForegroundColor Yellow
Write-Host "  [System.Environment]::SetEnvironmentVariable('PGPASSWORD', 'ваш_пароль', 'User')" -ForegroundColor White
Write-Host ""

# Очистить пароль из переменной (но оставить в $env:PGPASSWORD)
$password = $null

