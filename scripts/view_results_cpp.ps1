Write-Host "--- РЕЗУЛЬТАТЫ NoSQL КОНВЕРТАЦИИ (C++ / SQLite-KV) ---" -ForegroundColor Cyan
$DataDir = "C:\QT_projects\voenkomat_app\nosql_db_cpp"

if (Test-Path $DataDir) {
    $files = Get-ChildItem "$DataDir\*.db"
    if ($files.Count -eq 0) {
        Write-Host "Базы данных не найдены в $DataDir" -ForegroundColor Red
        return
    }

    foreach ($file in $files) {
        Write-Host "`n--------------------------------------------------" -ForegroundColor Gray
        Write-Host "DATABASE: $($file.BaseName)" -ForegroundColor Yellow
        Write-Host "File: $($file.Name) ($($file.Length) bytes)"

        # Пробуем прочитать первые 2 записи через sqlite3
        # Если sqlite3 нет в PATH, скрипт просто выведет ошибку, но не упадет
        try {
            Write-Host "Records (Key -> Value):" -ForegroundColor Cyan
            & sqlite3.exe $file.FullName "SELECT key || ' -> ' || value FROM kv;" 2>$null

            $count = & sqlite3.exe $file.FullName "SELECT count(*) FROM kv;" 2>$null
            Write-Host "Total records: $count" -ForegroundColor Green
        } catch {
            Write-Host "Утилита sqlite3.exe не найдена. Не удалось прочитать содержимое напрямую." -ForegroundColor Red
        }
    }
} else {
    Write-Host "Папка $DataDir не найдена. Запустите: .\run_converter_cpp.ps1" -ForegroundColor Red
}
