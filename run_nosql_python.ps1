Write-Host "--- ЗАПУСК NOSQL КОНВЕРТЕРА (PYTHON) ---" -ForegroundColor Cyan

# Путь к папке сервера Python
$PythonDir = "C:\QT_projects\voenkomat_app\server_python"

# Переходим в папку и запускаем конвертер
Push-Location $PythonDir
python converter.py
Pop-Location

Write-Host "`n--- ПРОВЕРКА РЕЗУЛЬТАТОВ (БИНАРНЫЕ ФАЙЛЫ) ---" -ForegroundColor Yellow
$NoSqlDir = "C:\QT_projects\voenkomat_app\nosql_db_python"

if (Test-Path $NoSqlDir) {
    Get-ChildItem $NoSqlDir -Filter "*.db" | ForEach-Object {
        Write-Host "Файл: $($_.Name), Размер: $($_.Length) байт"
    }
    Write-Host "`nПример данных из первой таблицы (дамп):"
    $firstTxt = Get-ChildItem $NoSqlDir -Filter "*.db.txt" | Select-Object -First 1
    if ($firstTxt) {
        Get-Content $firstTxt.FullName -TotalCount 5
    }
} else {
    Write-Host "Ошибка: Папка $NoSqlDir не создана." -ForegroundColor Red
}
