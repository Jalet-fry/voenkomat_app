# Прямые ссылки для скачивания PostgreSQL 12 (32-bit)

## Официальный FTP архив PostgreSQL

### PostgreSQL 12.18 (последняя версия 12)

**Windows x86 (32-bit) установщик:**
```
https://get.enterprisedb.com/postgresql/postgresql-12.18-1-windows-x86.exe
```

**Или через официальный FTP:**
```
https://ftp.postgresql.org/pub/source/v12.18/
```

---

## Альтернативные источники

### 1. SourceForge (может иметь старые версии)
```
https://sourceforge.net/projects/postgresql/
```

### 2. GitHub Releases (если есть)
Поищите: `postgresql windows x86 release`

### 3. Архивные зеркала
- https://archive.org/web/ - поищите старые версии страниц EDB

---

## Если ссылки не работают

**Попробуйте:**
1. Поиск в Google: `"postgresql-12" "windows-x86" download`
2. Поиск в Bing: `postgresql 12 32 bit windows installer`
3. Попросите на форумах: https://www.postgresql.org/community/

---

## Важно

После скачивания:
- Установите только "Command Line Tools" (не сервер)
- Скопируйте `libpq.dll` из `C:\Program Files (x86)\PostgreSQL\12\bin\`
- Вставьте в папку `C:\QT_projects\voenkomat_app\debug\`

