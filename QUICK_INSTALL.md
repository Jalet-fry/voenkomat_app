# Быстрая установка 32-bit libpq.dll

## Самый простой способ (3 шага)

### ШАГ 1: Скачать PostgreSQL 12 (32-bit)

**Прямая ссылка для скачивания:**
```
https://get.enterprisedb.com/postgresql/postgresql-12.18-1-windows-x86.exe
```

Или:
1. Откройте: https://www.postgresql.org/download/windows/
2. Найдите "PostgreSQL 12"
3. Скачайте версию с пометкой **"x86"** или **"32-bit"**
4. Файл должен называться: `postgresql-12.x-x-windows-x86.exe` (НЕ x64!)

---

### ШАГ 2: Установить (только клиент)

1. Запустите скачанный файл `.exe`

2. На экране **"Select Components"**:
   - ✅ Выберите: **Command Line Tools**
   - ❌ НЕ выбирайте: PostgreSQL Server

3. Нажмите "Next" → "Next" → "Install" → "Finish"

---

### ШАГ 3: Автоматическое копирование

**Запустите скрипт:**
```powershell
.\install_32bit_libpq.ps1
```

Скрипт автоматически:
- Найдет PostgreSQL 12/13
- Скопирует нужные файлы в папку `debug`
- Покажет результат

**Готово!** ✅

---

## Если скрипт не работает (ручной способ)

1. Откройте папку:
   ```
   C:\Program Files (x86)\PostgreSQL\12\bin\
   ```
   (Если нет, проверьте: `C:\Program Files\PostgreSQL\12\bin\`)

2. Скопируйте файл `libpq.dll`

3. Вставьте в папку:
   ```
   C:\QT_projects\voenkomat_app\debug\
   ```

4. Готово!

---

## Проверка

Запустите приложение - должно работать! 🎉

