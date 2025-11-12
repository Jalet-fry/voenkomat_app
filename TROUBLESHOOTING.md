# 🔧 Устранение проблем с подключением к базе данных

## Проблема: "Не удалось подключиться к базе данных"

### Шаг 1: Проверьте, запущен ли PostgreSQL

1. Откройте **Службы** (Win + R → `services.msc` → Enter)
2. Найдите службу PostgreSQL (обычно `postgresql-x64-17` или похожую)
3. Убедитесь, что статус **"Выполняется"**
4. Если не запущена, нажмите правой кнопкой → **Запустить**

### Шаг 2: Проверьте переменную окружения PGPASSWORD

Приложение использует переменную окружения `PGPASSWORD` для пароля.

#### В PowerShell (рекомендуется):
```powershell
# Проверить, установлена ли переменная
$env:PGPASSWORD

# Если пусто, установите её:
$env:PGPASSWORD = "ваш_пароль"

# Или используйте скрипт:
.\set_password.ps1
```

#### В CMD:
```cmd
REM Проверить
echo %PGPASSWORD%

REM Установить
set PGPASSWORD=ваш_пароль
```

**ВАЖНО:** Переменная окружения должна быть установлена **ДО** запуска приложения Qt!

### Шаг 3: Проверьте файл config.ini

Убедитесь, что файл `config.ini` существует в корне проекта и содержит правильные параметры:

```ini
[Database]
host=localhost
port=5432
database=voenkomat
username=postgres
```

### Шаг 4: Проверьте доступность драйвера QPSQL

Qt должен быть собран с поддержкой PostgreSQL. Проверьте:

1. Откройте Qt Creator
2. Перейдите в **Справка** → **О Qt Creator**
3. Проверьте, что драйвер QPSQL доступен

Если драйвер недоступен, вам нужно пересобрать Qt с поддержкой PostgreSQL или использовать другую версию Qt.

### Шаг 5: Проверьте параметры подключения

Убедитесь, что:
- **Хост:** `localhost` (или правильный IP-адрес)
- **Порт:** `5432` (или ваш порт PostgreSQL)
- **База данных:** `voenkomat` существует
- **Пользователь:** `postgres` (или ваш пользователь)
- **Пароль:** правильный пароль в переменной окружения

### Шаг 6: Проверьте подключение через psql

Попробуйте подключиться через командную строку:

```powershell
# Установите пароль
$env:PGPASSWORD = "ваш_пароль"

# Подключитесь
& "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d voenkomat
```

Если подключение через psql работает, а приложение не работает, проблема может быть в:
- Переменной окружения (не установлена в сессии, где запущено приложение)
- Драйвере QPSQL в Qt

## Типичные ошибки и решения

### Ошибка: "Драйвер QPSQL не доступен"
**Решение:** Пересоберите Qt с поддержкой PostgreSQL или используйте другую версию Qt.

### Ошибка: "Пароль не указан"
**Решение:** Установите переменную окружения `PGPASSWORD` перед запуском приложения.

### Ошибка: "could not connect to server"
**Решение:** 
1. Проверьте, запущен ли PostgreSQL сервер
2. Проверьте правильность хоста и порта в `config.ini`

### Ошибка: "password authentication failed"
**Решение:** 
1. Проверьте правильность пароля в переменной окружения `PGPASSWORD`
2. Убедитесь, что пользователь `postgres` существует и имеет правильный пароль

### Ошибка: "database does not exist"
**Решение:** 
1. Создайте базу данных `voenkomat`:
   ```sql
   CREATE DATABASE voenkomat;
   ```
2. Или измените имя базы данных в `config.ini`

## Быстрая проверка

Выполните этот скрипт для проверки всех параметров:

```powershell
# Проверка PostgreSQL
$pgService = Get-Service | Where-Object {$_.Name -like "*postgresql*"}
if ($pgService) {
    Write-Host "✓ PostgreSQL служба найдена: $($pgService.Name)" -ForegroundColor Green
    Write-Host "  Статус: $($pgService.Status)" -ForegroundColor $(if ($pgService.Status -eq 'Running') {'Green'} else {'Red'})
} else {
    Write-Host "✗ PostgreSQL служба не найдена" -ForegroundColor Red
}

# Проверка переменной окружения
if ($env:PGPASSWORD) {
    Write-Host "✓ PGPASSWORD установлена" -ForegroundColor Green
} else {
    Write-Host "✗ PGPASSWORD не установлена" -ForegroundColor Red
    Write-Host "  Выполните: \$env:PGPASSWORD = 'ваш_пароль'" -ForegroundColor Yellow
}

# Проверка config.ini
if (Test-Path "config.ini") {
    Write-Host "✓ config.ini найден" -ForegroundColor Green
} else {
    Write-Host "✗ config.ini не найден" -ForegroundColor Red
}

# Проверка psql
$psqlPath = "C:\Program Files\PostgreSQL\17\bin\psql.exe"
if (Test-Path $psqlPath) {
    Write-Host "✓ psql.exe найден" -ForegroundColor Green
} else {
    Write-Host "✗ psql.exe не найден по пути: $psqlPath" -ForegroundColor Red
}
```

## Получение помощи

Если проблема не решена:
1. Проверьте логи приложения (вывод в консоли Qt Creator)
2. Проверьте логи PostgreSQL (обычно в `C:\Program Files\PostgreSQL\17\data\log`)
3. Убедитесь, что все параметры подключения правильные

