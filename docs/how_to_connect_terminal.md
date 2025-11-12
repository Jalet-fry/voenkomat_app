# Пошаговая инструкция: Подключение к БД через терминал

## Шаг 1: Найти где установлен PostgreSQL

PostgreSQL обычно устанавливается в одну из этих папок:
- `C:\Program Files\PostgreSQL\[версия]\bin\`
- `C:\Program Files (x86)\PostgreSQL\[версия]\bin\`

### Способ 1: Через Проводник Windows

1. Откройте **Проводник Windows** (Win+E)
2. Перейдите в `C:\Program Files\PostgreSQL\`
3. Найдите папку с версией (например, `17`, `16`, `15`, `14` и т.д.)
4. Зайдите в папку `bin`
5. Найдите файл `psql.exe`

**Запомните полный путь**, например: `C:\Program Files\PostgreSQL\17\bin\psql.exe`

### Способ 2: Через поиск Windows

1. Нажмите **Win + S** (поиск Windows)
2. Введите: `psql`
3. Если найдется, нажмите "Открыть расположение файла"
4. Запомните путь

### Способ 3: Через PowerShell (автоматический поиск)

Откройте PowerShell и выполните:

```powershell
# Поиск psql.exe
Get-ChildItem -Path "C:\Program Files\PostgreSQL" -Recurse -Filter "psql.exe" -ErrorAction SilentlyContinue | Select-Object FullName
```

## Шаг 2: Открыть терминал (PowerShell)

1. Нажмите **Win + X**
2. Выберите **"Windows PowerShell"** или **"Терминал"**
3. Или нажмите **Win + R**, введите `powershell` и нажмите Enter

## Шаг 3: Перейти в папку с psql (если не в PATH)

Если `psql` не найден автоматически, нужно указать полный путь:

```powershell
# Замените [версия] на вашу версию PostgreSQL
cd "C:\Program Files\PostgreSQL\[версия]\bin"
```

**Пример:**
```powershell
cd "C:\Program Files\PostgreSQL\17\bin"
```

## Шаг 4: Подключиться к базе данных

### Вариант А: Подключение с указанием всех параметров

```powershell
# Замените [версия] на вашу версию
.\psql.exe -h localhost -p 5432 -U postgres -d voenkomat
```

**Что означает:**
- `-h localhost` - хост (обычно localhost)
- `-p 5432` - порт (обычно 5432)
- `-U postgres` - пользователь (обычно postgres)
- `-d voenkomat` - имя базы данных

### Вариант Б: Подключение с паролем через переменную окружения

```powershell
# Установить пароль (замените YOUR_PASSWORD на ваш пароль)
$env:PGPASSWORD = "YOUR_PASSWORD"

# Подключиться
.\psql.exe -h localhost -p 5432 -U postgres -d voenkomat
```

### Вариант В: Простое подключение (если psql в PATH)

Если вы добавили PostgreSQL в PATH, можно просто:

```powershell
psql -h localhost -p 5432 -U postgres -d voenkomat
```

## Шаг 5: Ввести пароль (если не использовали переменную)

После выполнения команды подключения, система попросит ввести пароль:

```
Password for user postgres:
```

**Введите пароль** (он не будет отображаться на экране) и нажмите Enter.

## Шаг 6: Проверить подключение

После успешного подключения вы увидите приглашение:

```
voenkomat=#
```

Это означает, что вы подключены к базе данных `voenkomat`.

### Полезные команды для проверки:

```sql
-- Показать версию PostgreSQL
SELECT version();

-- Показать текущую базу данных
SELECT current_database();

-- Показать текущего пользователя
SELECT current_user;

-- Показать все таблицы
\dt

-- Показать информацию о подключении
\conninfo

-- Выйти из psql
\q
```

## Шаг 7: Получить параметры подключения

Выполните этот SQL запрос:

```sql
SELECT 
    current_database() as database_name,
    current_user as username,
    inet_server_addr() as server_host,
    inet_server_port() as server_port,
    version() as postgresql_version;
```

## Добавление PostgreSQL в PATH (опционально)

Чтобы не указывать полный путь каждый раз:

1. Скопируйте путь к папке `bin` (например: `C:\Program Files\PostgreSQL\17\bin`)
2. Нажмите **Win + X** → **Система**
3. Нажмите **Дополнительные параметры системы**
4. Нажмите **Переменные среды**
5. В разделе **Системные переменные** найдите `Path`
6. Нажмите **Изменить**
7. Нажмите **Создать**
8. Вставьте путь: `C:\Program Files\PostgreSQL\17\bin`
9. Нажмите **ОК** везде
10. **Перезапустите PowerShell**

После этого можно использовать просто `psql` без полного пути.

## Пример полной сессии

```powershell
# 1. Открыть PowerShell
# 2. Перейти в папку bin (если не в PATH)
cd "C:\Program Files\PostgreSQL\17\bin"

# 3. Установить пароль
$env:PGPASSWORD = "ваш_пароль"

# 4. Подключиться
.\psql.exe -h localhost -p 5432 -U postgres -d voenkomat

# 5. После подключения выполнить SQL:
SELECT current_database(), current_user;

# 6. Выйти
\q
```

## Решение проблем

### Проблема: "psql не распознан"

**Решение:** Используйте полный путь к `psql.exe` или добавьте PostgreSQL в PATH.

### Проблема: "password authentication failed"

**Решение:** Проверьте правильность пароля. Можно сбросить пароль через pgAdmin.

### Проблема: "could not connect to server"

**Решение:** 
- Убедитесь, что PostgreSQL запущен
- Проверьте правильность хоста и порта
- Проверьте, не блокирует ли файрвол подключение

### Проблема: "database does not exist"

**Решение:** 
- Проверьте имя базы данных: `\l` (список всех баз)
- Создайте базу: `CREATE DATABASE voenkomat;`

## Быстрая команда для копирования

Если вы знаете путь к psql, используйте эту команду (замените путь и пароль):

```powershell
$env:PGPASSWORD = "ваш_пароль"
& "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d voenkomat
```

