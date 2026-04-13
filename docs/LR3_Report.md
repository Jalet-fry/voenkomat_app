Министерство образования Республики Беларусь

Учреждение образования
БЕЛОРУССКИЙ ГОСУДАРСТВЕННЫЙ УНИВЕРСИТЕТ
ИНФОРМАТИКИ И РАДИОЭЛЕКТРОНИКИ

Факультет компьютерных систем и сетей
Кафедра электронных вычислительных машин
<br><br>

Дисциплина: Базы данных

<br><br><br><br><br><br>

# ОТЧЕТ
### по лабораторной работе №3
### «Разработка NoSQL базы данных и спецификаций прикладной программы»
### на тему: «ИНФОРМАЦИОННАЯ СИСТЕМА ВОЕНКОМАТ»

<br><br><br><br><br><br><br><br>

**Студент:** В.А. Волосевич  
**Преподаватель:** С.С. Силич  

<br><br><br><br><br><br>
Минск 2026

---
<br><br>

## СОДЕРЖАНИЕ

1. [ВВЕДЕНИЕ](#введение)
2. [1 РАЗРАБОТКА КОНВЕРТОРА](#1-разработка-конвертора)
3. [2 РАЗРАБОТКА ПРИЛОЖЕНИЯ](#2-разработка-приложения)
    * [2.1 Структура BerkeleyDB](#21-структура-berkeleydb)
4. [ЗАКЛЮЧЕНИЕ](#заключение)
5. [ПРИЛОЖЕНИЕ А](#приложение-а)

---
<br><br>

## ВВЕДЕНИЕ

Современные тенденции в области управления данными демонстрируют растущий интерес к альтернативным системам хранения информации, которые выходят за рамки традиционных реляционных подходов. Лабораторная работа №3 посвящена изучению принципов организации NoSQL базы данных на основе СУБД BerkeleyDB (реализованной через интерфейс dbm в языке Python и текстовые дампы в C++), а также разработке конвертора для миграции данных.

Целью работы является разработка консольного приложения-конвертора, осуществляющего трансформацию данных из реляционной базы данных PostgreSQL в набор баз данных BerkeleyDB формата «ключ-значение». Использование NoSQL подхода позволяет обеспечить высокую скорость доступа к данным по первичному ключу и гибкость хранения за счет использования формата JSON.

---
<br><br>

## 1 РАЗРАБОТКА КОНВЕРТОРА

В таблице 1.1 приведено описание формата хранения данных, конвертированных из таблиц PostgreSQL в базы данных BerkeleyDB. В полях таблицы приведены названия столбцов таблиц PostgreSQL.

**Таблица 1.1 – Формат хранения данных в BerkeleyDB**

| Таблица PostgreSQL | BerkeleyDB Ключ | Значение (JSON) |
| :--- | :--- | :--- |
| **comissar** | id_comissar | {fio, dolzhnost, stazh_raboty, kontaktnyi_telefon} |
| **kategoria_godnosti** | id_kategorii | {nazvanie_kategorii, opisanie_ogranichenii, index_kategorii...} |
| **med_osvidetelstvovanie** | id_osvidetelstvovania | {data_provedeniya, rezultaty_obsledovania, fio_vracha, zaklyuchenie, id_prizivnika, id_kategorii} |
| **prizivnik** | id_prizivnik | {fio, data_rozhdeniya, adres_prozhivaniya, nomer_pasporta, id_voennogo_bileta, id_voenno_uchetnoi_karty} |
| **prizivnoe_meropriyatie** | id_meropriyatiya | {tip_meropriyatiya, data_provedeniya, mesto_provedeniya, fio_comissara, id_comissar} |
| **voenno_uchetnaya_karta** | id_karty | {nomer_karty, data_postanovki_na_uchet, istoriya_otsrochek, voenno_uchetnaya_specialnost, id_prizivnika} |
| **voennyi_bilet** | id_bileta | {nomer_bileta, data_vydachi, voinskoe_zvanie, kategoria, id_prizivnika, id_kategorii} |
| **prizivnik_comissar** | {id_prizivnik}_{id_comissar} | {id_prizivnik, id_comissar, data_vzaimodeistviya, nomer_kabineta} |
| **prizivnik_meropriyatie** | {id_prizivnik}_{id_meropriyatiya} | {id_prizivnik, id_meropriyatiya} |

<br>

### Алгоритм работы конвертора:

1.  **Установка соединения:** Подключение к локальному серверу PostgreSQL с использованием параметров аутентификации.
2.  **Динамическое сканирование схемы:** Выполнение запроса к `information_schema.tables` для получения списка всех пользовательских таблиц в схеме `public`.
3.  **Обработка каждой таблицы:**
    *   **Анализ метаданных:** Получение списка столбцов и определение структуры первичного ключа.
    *   **Извлечение данных:** Чтение всех строк из таблицы PostgreSQL.
    *   **Трансформация ключа:** Если первичный ключ простой — используется его значение. Если составной (для M2M связей) — формируется строка вида `key1_key2`.
    *   **Сериализация:** Формирование JSON-объекта из остальных полей строки. Обработка типов данных `date`, `timestamp` и `numeric` с помощью кастомного энкодера.
    *   **Запись в NoSQL:** Открытие (или создание) файла `.db` и сохранение пары «ключ-значение».
4.  **Завершение:** Закрытие дескрипторов баз данных и разрыв соединения с PostgreSQL.

<br>

### Основные SQL-запросы реализации:

**1. Запрос для получения имен всех таблиц схемы public:**
```sql
SELECT table_name 
FROM information_schema.tables 
WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
ORDER BY table_name;
```
Этот запрос обеспечивает универсальность конвертора: при добавлении новых таблиц в БД «Военкомат» они будут автоматически включены в процесс миграции без изменения кода.

**2. Запрос для получения структуры первичных ключей:**
```sql
SELECT kcu.column_name 
FROM information_schema.table_constraints tc 
JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = %s
ORDER BY kcu.ordinal_position;
```
Позволяет корректно обрабатывать таблицы связей (например, `prizivnik_comissar`), где ключ является составным.

**3. Аналитический запрос (Призывники и их статус годности):**
```sql
SELECT p.fio, k.nazvanie_kategorii, m.zaklyuchenie
FROM prizivnik p
JOIN med_osvidetelstvovanie m ON p.id_prizivnik = m.id_prizivnika
JOIN kategoria_godnosti k ON m.id_kategorii = k.id_kategorii;
```
Используется для проверки консистентности данных после конвертации.

---
<br><br>

## 2 РАЗРАБОТКА ПРИЛОЖЕНИЯ

В ходе выполнения работы были разработаны два варианта конвертора: на языке Python (с использованием библиотеки `dbm`) и на языке C++ (с использованием `QtSql` и `QJson`).

### 2.1 Структура BerkeleyDB

Архитектура NoSQL базы данных в данном проекте основана на принципе **Key-Value Store**. В отличие от реляционной модели, где данные распределены по нормализованным таблицам, здесь каждая запись представляет собой самодостаточный объект.

**Логическая организация:**
*   **Ключ (Key):** Уникальный идентификатор. Для обеспечения связности данных ключи в BerkeleyDB в точности соответствуют значениям Primary Key из PostgreSQL. Для таблиц M2M используется конкатенация ключей через разделитель.
*   **Значение (Value):** Текстовая строка в формате JSON. JSON позволяет хранить вложенные структуры и легко расширять набор полей без изменения схемы базы данных (Schema-less).

**Физическая структура:**
Все сгенерированные базы данных располагаются в директории `server_python/nosql_db`. Для каждой таблицы создается файл с расширением `.db`.

---
<br><br>

## ЗАКЛЮЧЕНИЕ

В результате выполнения лабораторной работы была успешно спроектирована и реализована система миграции данных из реляционной СУБД PostgreSQL в NoSQL хранилище BerkeleyDB.

В ходе работы:
1.  Освоены методы работы с `information_schema` для динамического анализа структуры БД.
2.  Реализована логика формирования составных ключей для сохранения связей «многие-ко-многим».
3.  Разработан кастомный механизм сериализации данных, обеспечивающий корректный перенос типов дат и времени.

Использование BerkeleyDB в качестве вторичного хранилища позволяет приложению «Военкомат» эффективно выполнять поиск информации по ID без нагрузки на основной сервер PostgreSQL.

---
<br><br>

## ПРИЛОЖЕНИЕ А
**(обязательное)**  
### Листинг кода

**Файл: server_python/converter.py**

```python
import json
import os
import dbm
from datetime import datetime, date, time
from decimal import Decimal

# Кастомный энкодер для обработки типов данных PostgreSQL в JSON
class NoSQLJSONEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (datetime, date, time)):
            return obj.isoformat()
        elif isinstance(obj, Decimal):
            return float(obj)
        return super().default(obj)

def convert():
    print("--- ЛАБОРАТОРНАЯ РАБОТА №3: КОНВЕРТАЦИЯ В NoSQL ---")
    
    nosql_dir = "nosql_db"
    if not os.path.exists(nosql_dir):
        os.makedirs(nosql_dir)

    # Шаг 1: Получение имен всех таблиц
    tables_res = execute_query("""
        SELECT table_name 
        FROM information_schema.tables 
        WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
    """)
    tables = [row['table_name'] for row in tables_res]

    for table_name in tables:
        print(f"Обработка таблицы: {table_name}...")
        
        # Шаг 2: Определение структуры Primary Key
        pk_query = f"""
            SELECT kcu.column_name 
            FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
            ORDER BY kcu.ordinal_position
        """
        pk_cols = [row['column_name'] for row in pk_res] if (pk_res := execute_query(pk_query)) else []

        # Шаг 3: Получение данных
        data = execute_query(f"SELECT * FROM public.{table_name}")
        db_path = os.path.join(nosql_dir, f"{table_name}.db")
        
        # Шаг 4: Запись в BerkeleyDB (dbm)
        try:
            with dbm.open(db_path, 'n') as db:
                for row in data:
                    # Формирование КЛЮЧА
                    if len(pk_cols) > 1:
                        # Составной ключ для M2M
                        key_str = "_".join([str(row[c]) for c in pk_cols])
                    elif len(pk_cols) == 1:
                        # Простой ключ
                        key_str = str(row[pk_cols[0]])
                    else:
                        # Фолбэк на первую колонку
                        key_str = str(list(row.values())[0])

                    # Формирование ЗНАЧЕНИЯ (JSON)
                    # Если PK один, исключаем его из JSON для экономии места
                    if len(pk_cols) == 1:
                        val_dict = {k: v for k, v in row.items() if k not in pk_cols}
                    else:
                        val_dict = row

                    db[key_str] = json.dumps(val_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)
                
                print(f"  Успешно: {table_name}.db создана.")
        except Exception as e:
            print(f"  Ошибка при записи {table_name}: {e}")

    print("\n--- КОНВЕРТАЦИЯ ЗАВЕРШЕНА УСПЕШНО ---")

if __name__ == "__main__":
    convert()
```

<br><br>

**Файл: server_cpp/Converter.cpp**

```cpp
#include "Converter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlRecord>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>

void Converter::run()
{
    qInfo() << "--- ЛАБОРАТОРНАЯ РАБОТА №3: NoSQL КОНВЕРТЕР (C++) ---";

    QSqlDatabase db = QSqlDatabase::database();
    QString nosqlDir = "nosql_db_cpp";
    QDir().mkpath(nosqlDir);

    QSqlQuery tableQuery(db);
    tableQuery.prepare("SELECT table_name FROM information_schema.tables "
                       "WHERE table_schema = 'public' AND table_type = 'BASE TABLE'");

    if (!tableQuery.exec()) return;

    while (tableQuery.next()) {
        QString tableName = tableQuery.value(0).toString();
        QStringList pkCols = getPrimaryKeyColumns(tableName);

        QSqlQuery dataQuery(db);
        dataQuery.prepare(QString("SELECT * FROM public.%1").arg(tableName));
        if (!dataQuery.exec()) continue;

        QSqlRecord rec = dataQuery.record();
        QFile dbFile(QDir(nosqlDir).filePath(tableName + ".db.txt"));
        
        if (dbFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&dbFile);
            while (dataQuery.next()) {
                QJsonObject valObj;
                QString keyStr;

                // Генерация ключа
                if (pkCols.size() > 1) {
                    QStringList parts;
                    for (const auto &pk : pkCols) parts << dataQuery.value(pk).toString();
                    keyStr = parts.join("_");
                } else if (!pkCols.isEmpty()) {
                    keyStr = dataQuery.value(pkCols[0]).toString();
                }

                // Генерация JSON
                for (int i = 0; i < rec.count(); ++i) {
                    QString name = rec.fieldName(i);
                    if (pkCols.size() == 1 && name == pkCols[0]) continue;
                    valObj[name] = QJsonValue::fromVariant(dataQuery.value(i));
                }

                out << keyStr << " ||| " << QJsonDocument(valObj).toJson(QJsonDocument::Compact) << "\n";
            }
            dbFile.close();
        }
    }
}
```
