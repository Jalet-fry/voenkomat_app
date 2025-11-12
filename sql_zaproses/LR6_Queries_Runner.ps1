# PowerShell скрипт для последовательного выполнения всех SQL запросов из LR6
# Использование: .\LR6_Queries_Runner.ps1

$queriesPath = Join-Path $PSScriptRoot "LR6_Queries"
$pauseSeconds = 3

# Параметры подключения к БД (измените под ваши настройки)
$dbHost = "localhost"
$dbPort = "5432"
$dbName = "voenkomat"
$dbUser = "postgres"
# Для пароля используйте переменную окружения PGPASSWORD или .pgpass файл

Write-Host "=== Начало выполнения всех запросов LR6 ===" -ForegroundColor Green
Write-Host "Папка с запросами: $queriesPath" -ForegroundColor Yellow
Write-Host "Пауза между запросами: $pauseSeconds секунд" -ForegroundColor Yellow
Write-Host ""

# Получаем все SQL файлы в папке, отсортированные по имени
$queryFiles = Get-ChildItem -Path $queriesPath -Filter "*.sql" | Sort-Object Name

if ($queryFiles.Count -eq 0) {
    Write-Host "ОШИБКА: Не найдены SQL файлы в папке $queriesPath" -ForegroundColor Red
    exit 1
}

Write-Host "Найдено запросов: $($queryFiles.Count)" -ForegroundColor Cyan
Write-Host ""

$successCount = 0
$errorCount = 0

foreach ($file in $queryFiles) {
    $queryNumber = $file.BaseName
    Write-Host "[$queryNumber] Выполняется запрос из файла: $($file.Name)" -ForegroundColor Cyan
    
    # Формируем команду psql
    $sqlContent = Get-Content $file.FullName -Raw -Encoding UTF8
    
    # Выполняем запрос через psql
    # Для пароля используйте переменную окружения PGPASSWORD или .pgpass файл
    # $env:PGPASSWORD = "your_password_here"
    
    try {
        # Альтернативный способ: сохранение во временный файл и выполнение
        $tempFile = [System.IO.Path]::GetTempFileName() + ".sql"
        $sqlContent | Out-File -FilePath $tempFile -Encoding UTF8
        
        $psqlCmd = "psql -h $dbHost -p $dbPort -U $dbUser -d $dbName -f `"$tempFile`""
        $result = Invoke-Expression $psqlCmd 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "[$queryNumber] ✓ Успешно выполнено" -ForegroundColor Green
            $successCount++
        } else {
            Write-Host "[$queryNumber] ✗ ОШИБКА при выполнении" -ForegroundColor Red
            Write-Host $result -ForegroundColor Red
            $errorCount++
        }
        
        Remove-Item $tempFile -ErrorAction SilentlyContinue
    }
    catch {
        Write-Host "[$queryNumber] ✗ ИСКЛЮЧЕНИЕ: $_" -ForegroundColor Red
        $errorCount++
    }
    
    # Пауза между запросами
    if ($file -ne $queryFiles[-1]) {
        Write-Host "Пауза $pauseSeconds секунд..." -ForegroundColor Gray
        Start-Sleep -Seconds $pauseSeconds
    }
    
    Write-Host ""
}

Write-Host "=== Результаты выполнения ===" -ForegroundColor Green
Write-Host "Успешно: $successCount" -ForegroundColor Green
Write-Host "Ошибок: $errorCount" -ForegroundColor $(if ($errorCount -gt 0) { "Red" } else { "Green" })

