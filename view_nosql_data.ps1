# Скрипт для просмотра содержимого Berkeley DB через db_dump
$BdbBin = "C:\Program Files (x86)\Oracle\Berkeley DB 12cR1 6.0.30\bin\db_dump.exe"
$DataDir = "C:\QT_projects\voenkomat_app\nosql_db_cpp"

if (-not (Test-Path $BdbBin)) {
    Write-Host "[ERROR] db_dump.exe не найден по пути: $BdbBin" -ForegroundColor Red
    Write-Host "Убедитесь, что Berkeley DB установлена корректно."
    exit
}

if (-not (Test-Path $DataDir)) {
    Write-Host "[ERROR] Директория с данными не найдена: $DataDir" -ForegroundColor Red
    exit
}

$dbFiles = Get-ChildItem "$DataDir\*.db"
if ($dbFiles.Count -eq 0) {
    Write-Host "[WARNING] Файлы .db не найдены в $DataDir. Сначала запустите конвертер." -ForegroundColor Yellow
    exit
}

Write-Host "--- ПРОСМОТР СОДЕРЖИМОГО BERKELEY DB ---" -ForegroundColor Magent

foreach ($file in $dbFiles) {
    Write-Host "`n[FILE] $($file.Name)" -ForegroundColor Cyan
    Write-Host ("-" * 30)
    # -p делает данные читаемыми (если они текстовые/JSON)
    & $BdbBin -p $file.FullName
}

Write-Host "`n--- КОНЕЦ ДАМПА ---" -ForegroundColor Magenta
