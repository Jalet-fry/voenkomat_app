# Установка SQL драйверов для Qt приложения

## Проблема: "QPSQL driver not loaded"

Если вы видите ошибку "Driver not loaded", это означает, что Qt не может найти плагины SQL драйверов.

## Решение 1: Копирование драйверов в папку приложения (РЕКОМЕНДУЕТСЯ)

### Шаг 1: Найдите драйверы Qt

Драйверы находятся в папке Qt:
```
C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers\
```

В этой папке должны быть файлы:
- `qsqlpsql.dll` (для PostgreSQL)
- `qsqlpsqld.dll` (отладочная версия)

### Шаг 2: Создайте структуру папок

В папке `debug` (где находится `voenkomat_app.exe`) создайте структуру:
```
debug/
  ├── voenkomat_app.exe
  └── plugins/
      └── sqldrivers/
          ├── qsqlpsql.dll
          └── qsqlpsqld.dll
```

### Шаг 3: Скопируйте файлы

**Для Debug сборки:**
1. Скопируйте из: `C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers\qsqlpsqld.dll`
2. В папку: `C:\QT_projects\voenkomat_app\debug\plugins\sqldrivers\`

**Для Release сборки:**
1. Скопируйте из: `C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers\qsqlpsql.dll`
2. В папку: `C:\QT_projects\voenkomat_app\release\plugins\sqldrivers\`

### Шаг 4: Скопируйте зависимости PostgreSQL

Драйвер QPSQL требует библиотеки PostgreSQL. Скопируйте из:
```
C:\Program Files\PostgreSQL\17\bin\
```

Скопируйте в папку `debug` (рядом с exe):
- `libpq.dll`
- `libintl-8.dll` (если есть)
- `libiconv-2.dll` (если есть)

## Решение 2: Использование скрипта PowerShell

Создайте файл `copy_drivers.ps1`:

```powershell
# Создаем структуру папок
$pluginsDir = "debug\plugins\sqldrivers"
New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null

# Копируем драйвер PostgreSQL (debug версия)
$sourceDriver = "C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\sqldrivers\qsqlpsqld.dll"
$destDriver = "$pluginsDir\qsqlpsqld.dll"
Copy-Item $sourceDriver $destDriver -Force
Write-Host "Скопирован драйвер: $destDriver" -ForegroundColor Green

# Копируем зависимости PostgreSQL
$pgBin = "C:\Program Files\PostgreSQL\17\bin"
$dlls = @("libpq.dll", "libintl-8.dll", "libiconv-2.dll")
foreach ($dll in $dlls) {
    $source = Join-Path $pgBin $dll
    if (Test-Path $source) {
        Copy-Item $source "debug\" -Force
        Write-Host "Скопирована зависимость: $dll" -ForegroundColor Green
    }
}

Write-Host "`nГотово! Драйверы скопированы." -ForegroundColor Green
```

Запустите скрипт в PowerShell из папки проекта.

## Проверка

После копирования файлов:
1. Пересоберите проект
2. Запустите приложение
3. Проверьте вывод - должно быть: "Добавлен путь к плагинам Qt: ..."

## Альтернативное решение: Установка переменной окружения QT_PLUGIN_PATH

Если не хотите копировать файлы, можно установить переменную окружения:

1. Откройте "Переменные среды" (Win+R → `sysdm.cpl` → Дополнительно → Переменные среды)
2. Создайте новую переменную:
   - Имя: `QT_PLUGIN_PATH`
   - Значение: `C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins`
3. Перезапустите Qt Creator и приложение

