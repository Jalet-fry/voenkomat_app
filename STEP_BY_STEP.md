# 📝 Пошаговая инструкция: Подключение к БД

## Шаг 1: Откройте PowerShell

1. Нажмите **Win + X**
2. Выберите **"Windows PowerShell"** или **"Терминал"**

Или:
- Нажмите **Win + R**
- Введите `powershell`
- Нажмите **Enter**

---

## Шаг 2: Перейдите в папку проекта

```powershell
cd "C:\Универ\SEM_5\БД\voenkomat_app"
```

---

## Шаг 3: Установите пароль

**ВАЖНО:** Замените `YOUR_PASSWORD` на ваш реальный пароль!

```powershell
$env:PGPASSWORD = "YOUR_PASSWORD"
```

**Пример:**
```powershell
$env:PGPASSWORD = "mypassword123"
```

⚠️ **Внимание:** Пароль не будет отображаться на экране - это нормально!

---

## Шаг 4: Подключитесь к базе данных

```powershell
& "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d voenkomat
```

---

## Шаг 5: Что вы увидите

Если подключение успешно, вы увидите:

```
psql (17.x)
Type "help" for help.

voenkomat=#
```

Это означает, что вы **успешно подключены**! 🎉

---

## Шаг 6: Выполните SQL запросы

Теперь вы можете выполнять SQL команды:

```sql
-- Проверить подключение
SELECT current_database(), current_user;

-- Показать все таблицы
\dt

-- Показать информацию о подключении
\conninfo

-- Выполнить тестовый запрос
SELECT COUNT(*) FROM prizivnik;
```

---

## Шаг 7: Выйти из psql

Введите:
```
\q
```

Или нажмите **Ctrl + D**

---

## 🚀 Быстрый способ (скрипт)

Вместо ручного ввода, вы можете использовать готовый скрипт:

1. Откройте файл `test_connection.ps1` в текстовом редакторе
2. Найдите строку: `$env:PGPASSWORD = "YOUR_PASSWORD";`
3. Замените `YOUR_PASSWORD` на ваш реальный пароль
4. Сохраните файл
5. В PowerShell выполните:

```powershell
.\test_connection.ps1
```

---

## ❌ Если не работает

### Ошибка: "password authentication failed"

**Причина:** Неправильный пароль

**Решение:** 
- Проверьте правильность пароля
- Попробуйте подключиться через pgAdmin, чтобы убедиться в правильности пароля

### Ошибка: "could not connect to server"

**Причина:** PostgreSQL не запущен

**Решение:**
1. Нажмите **Win + R**
2. Введите `services.msc`
3. Найдите службу **postgresql-x64-17** (или похожую)
4. Убедитесь, что она **Запущена**
5. Если не запущена - нажмите **Запустить**

### Ошибка: "database does not exist"

**Причина:** База данных `voenkomat` не существует

**Решение:**
1. Подключитесь к базе `postgres`:
   ```powershell
   & "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d postgres
   ```
2. Создайте базу данных:
   ```sql
   CREATE DATABASE voenkomat;
   ```
3. Выйдите: `\q`
4. Восстановите из бэкапа:
   ```powershell
   & "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d voenkomat -f "sql_zaproses\new_backup.sql"
   ```

---

## 📋 Полный пример сессии

```powershell
# 1. Открыть PowerShell
# 2. Перейти в папку проекта
cd "C:\Универ\SEM_5\БД\voenkomat_app"

# 3. Установить пароль (ЗАМЕНИТЕ на ваш!)
$env:PGPASSWORD = "ваш_пароль"

# 4. Подключиться
& "C:\Program Files\PostgreSQL\17\bin\psql.exe" -h localhost -p 5432 -U postgres -d voenkomat

# 5. В psql выполнить:
SELECT current_database(), current_user;

# 6. Выйти
\q
```

---

## 💡 Совет

После успешного подключения, **запомните или сохраните пароль** - он понадобится для создания конфигурационного файла `config.ini` в приложении!

