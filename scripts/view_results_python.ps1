Write-Host "--- ПОЛНЫЕ РЕЗУЛЬТАТЫ КОНВЕРТАЦИИ PYTHON (SQLite/NoSQL .db) ---" -ForegroundColor Cyan
$DataDir = "C:\QT_projects\voenkomat_app\nosql_db_python"

if (!(Test-Path $DataDir)) {
    Write-Host "Папка $DataDir не найдена!" -ForegroundColor Red
    exit
}

# Теперь мы ищем основные .db файлы
$dbFiles = Get-ChildItem "$DataDir\*.db" | Where-Object { $_.Extension -eq ".db" }

foreach ($dbFile in $dbFiles) {
    $tableName = $dbFile.BaseName
    Write-Host "`n=== ТАБЛИЦА: $tableName ===" -ForegroundColor Green

    # Пытаемся вывести данные из SQLite файла через PowerShell (если установлен sqlite3)
    # Если sqlite3.exe нет в PATH, скрипт просто покажет список файлов.
    if (Get-Command sqlite3 -ErrorAction SilentlyContinue) {
        Write-Host "Содержимое (Ключ ||| Значение):" -ForegroundColor Gray
        sqlite3.exe $dbFile.FullName "SELECT key || ' ||| ' || value FROM kv LIMIT 10;"
        $count = sqlite3.exe $dbFile.FullName "SELECT count(*) FROM kv;"
        Write-Host "Всего записей в БД: $count" -ForegroundColor Gray
    } else {
        Write-Host "Файл: $($dbFile.Name)" -ForegroundColor Gray
        Write-Host "Подсказка: Установите sqlite3 для просмотра содержимого через этот скрипт." -ForegroundColor Yellow
    }

    # Также проверяем текстовый дамп, если он был создан конвертером
    $txtFile = "$DataDir\$($tableName).db.txt"
    if (Test-Path $txtFile) {
        Write-Host "--- Текстовый дамп (.db.txt) ---" -ForegroundColor DarkGray
        $content = Get-Content $txtFile -TotalCount 10
        $content | ForEach-Object { Write-Host $_ }
        $total = (Get-Content $txtFile).Count
        if ($total -gt 10) { Write-Host "... и еще $($total - 10) записей" -ForegroundColor DarkGray }
    }
}
