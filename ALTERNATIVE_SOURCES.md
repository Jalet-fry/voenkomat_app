# Альтернативные источники для 32-bit libpq.dll

## Проблема
EDB больше не предоставляет 32-bit установщики для PostgreSQL 13+ на Windows.

## Решения

### Вариант 1: Скачать старую версию PostgreSQL 12 (32-bit)

**Официальный архив PostgreSQL:**
1. Перейдите на: https://www.postgresql.org/ftp/binary/v12.18/windows/
2. Найдите файл: `postgresql-12.18-1-windows-x86.exe` или `postgresql-12.18-1-windows-x86-binaries.zip`
3. Скачайте и установите (только Command Line Tools)

**Или через Wayback Machine:**
- https://web.archive.org/web/*/https://www.enterprisedb.com/downloads/postgres-postgresql-downloads
- Найдите старую версию страницы с PostgreSQL 12 (32-bit)

---

### Вариант 2: Готовые бинарники libpq.dll

**Прямые ссылки на готовые файлы (если доступны):**
- https://github.com/postgres/postgres/tree/master/src/interfaces/libpq
- Или поищите в интернете: "postgresql 12 libpq.dll 32-bit download"

---

### Вариант 3: Использовать готовый архив

**Скачать только нужные файлы:**
1. Найдите архив: `postgresql-12.18-1-windows-x86-binaries.zip`
2. Распакуйте
3. Скопируйте из папки `bin`: `libpq.dll`, `libiconv-2.dll`, `libintl-8.dll`

---

### Вариант 4: Попросить у коллег/друзей

Если у кого-то есть установленный PostgreSQL 12 (32-bit), попросите скопировать файлы из:
```
C:\Program Files (x86)\PostgreSQL\12\bin\
```

---

## Быстрое решение: Поиск в интернете

**Поисковые запросы:**
- "postgresql 12 windows x86 installer download"
- "postgresql 12 32-bit windows archive"
- "libpq.dll 32-bit postgresql 12"

**Рекомендуемые сайты:**
- https://www.postgresql.org/ftp/ (официальный FTP архив)
- https://sourceforge.net/projects/postgresql/ (может иметь старые версии)

---

## После получения файлов

1. Скопируйте `libpq.dll` в папку `debug`
2. Запустите приложение
3. Должно работать!

---

## Если ничего не помогает

Можно попробовать:
1. Собрать libpq.dll из исходников (сложно)
2. Использовать альтернативный драйвер (ODBC вместо QPSQL)
3. Перекомпилировать приложение как 64-bit (требует Qt 64-bit)

