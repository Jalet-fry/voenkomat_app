<div align="center">
    <p>Министерство образования Республики Беларусь</p>
    <p>Учреждение образования<br/>
    <strong>БЕЛОРУССКИЙ ГОСУДАРСТВЕННЫЙ УНИВЕРСИТЕТ<br/>
    ИНФОРМАТИКИ И РАДИОЭЛЕКТРОНИКИ</strong></p>
    <p>Факультет компьютерных систем и сетей<br/>
    Кафедра электронных вычислительных машин</p>
    <br/><br/><br/>
    <p>Дисциплина: <strong>Базы данных</strong></p>
    <br/><br/><br/><br/>
    <h2 style="margin-bottom: 0;">ОТЧЕТ</h2>
    <p style="margin-top: 0;">по лабораторной работе №3</p>
    <p><strong>«Разработка NoSQL базы данных и спецификаций прикладной программы»</strong></p>
    <p>на тему:</p>
    <h3 style="text-transform: uppercase; letter-spacing: 2px;">Информационная система «Военкомат»</h3>
    <br/><br/><br/><br/>
</div>

<div align="right" style="margin-right: 50px; font-size: 1.1em;">
    <p><strong>Студент:</strong> В.А. Волосевич</p>
    <p><strong>Преподаватель:</strong> С.С. Силич</p>
</div>

<div align="center" style="margin-top: 150px;">
    <p>Минск 2026</p>
</div>

---

### СОДЕРЖАНИЕ

1.  **[ВВЕДЕНИЕ](#введение)** ................................................................................................ 3
2.  **[1 РАЗРАБОТКА КОНВЕРТОРА](#разработка-конвертора)** ............................................ 4
    *   1.1 Спецификация хранения данных ...................................................................... 4
    *   1.2 Алгоритм работы конвертора ........................................................................... 5
    *   1.3 SQL-запросы для извлечения данных ................................................................ 6
3.  **[2 РАЗРАБОТКА ПРИЛОЖЕНИЯ](#разработка-приложения)** ........................................... 7
    *   2.1 Логическая и физическая структура NoSQL ....................................................... 7
    *   2.2 Анализ результатов выполнения ....................................................................... 8
4.  **[ЗАКЛЮЧЕНИЕ](#заключение)** ......................................................................................... 9
5.  **[ПРИЛОЖЕНИЕ А. ЛИСТИНГ КОДА](#приложение-а)** ..................................................... 10

---

<div id="введение"></div>

### ВВЕДЕНИЕ

Современные тенденции в области управления данными демонстрируют растущий интерес к альтернативным системам хранения информации, которые выходят за рамки традиционных реляционных подходов. Лабораторная работа №3 посвящена изучению принципов организации NoSQL базы данных на основе СУБД **BerkeleyDB** (реализованной через интерфейс `dbm` в языке Python), а также разработке конвертора для миграции данных.

**Целью работы** является разработка консольного приложения-конвертора, осуществляющего трансформацию данных из реляционной базы данных PostgreSQL в набор баз данных BerkeleyDB формата «ключ-значение».

---

<div id="разработка-конвертора"></div>

### 1 РАЗРАБОТКА КОНВЕРТОРА

#### 1.1 Спецификация хранения данных

В таблице 1.1 приведено описание формата хранения данных, конвертированных из таблиц PostgreSQL в базы данных BerkeleyDB.

**Таблица 1.1 – Формат хранения данных в BerkeleyDB**

| Таблица PostgreSQL | BerkeleyDB Ключ | Значение (JSON) |
| :--- | :--- | :--- |
| `conscripts` | `conscript_id` | `{full_name, birth_date, residence_address, passport_number, military_ticket_id, registration_card_id}` |
| `fitness_categories` | `category_id` | `{category_name, restriction_description, category_index, category_basis}` |
| `medical_examinations` | `certification_id` | `{examination_date, examination_results, doctor_full_name, conclusion, conscript_id, category_id}` |
| `commissioners` | `commissioner_id` | `{full_name, position, years_of_service, phone_number}` |
| `military_id_cards` | `ticket_id` | `{ticket_number, issue_date, military_rank, category, conscript_id, category_id}` |
| `service_record_cards` | `card_id` | `{card_number, registration_date, deferment_history, military_specialty, conscript_id}` |
| `conscripts_commissioners` | `{conscript_id}_{commissioner_id}` | `{interaction_date, office_number}` |
| `callup_events` | `event_id` | `{event_type, event_datetime, event_location, commissioner_full_name, commissioner_id}` |
| `conscripts_events` | `{conscript_id}_{event_id}` | `{}` |
| `test_table` | `id` | `{column_2, column_3}` |

#### 1.2 Алгоритм работы конвертора

1.  Установка соединения с локальным сервером PostgreSQL.
2.  Динамическое получение списка таблиц из схемы `public` через `information_schema.tables`.
3.  Для каждой таблицы:
    a. Получение списка столбцов и определение первичного ключа (PK).
    b. Извлечение всех записей.
    c. Сериализация данных в JSON (с обработкой типов `date` и `datetime` через `isoformat()`).
    d. Формирование строкового ключа (для составных PK — через разделитель `_`).
    e. Создание файла `.db` и запись пары ключ-значение.
4.  Закрытие соединений.

#### 1.3 SQL-запросы для извлечения данных

Для функционирования конвертора и аналитических модулей системы используются следующие запросы:

1.  **Запрос для динамического формирования списка сущностей**
    ```sql
    SELECT table_name FROM information_schema.tables 
    WHERE table_schema = 'public' AND table_type = 'BASE TABLE';
    ```
    **Реализация:**
    ```python
    def get_public_tables():
        tables_res = execute_query("""
            SELECT table_name FROM information_schema.tables 
            WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
        """)
        return [row['table_name'] for row in tables_res]
    ```

2.  **Запрос для определения первичных ключей (PK)**
    ```sql
    SELECT kcu.column_name 
    FROM information_schema.table_constraints tc 
    JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
    WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = 'table_name'
    ORDER BY kcu.ordinal_position;
    ```
    **Реализация:**
    ```python
    def get_pk_columns(table_name):
        pk_res = execute_query(f"""
            SELECT kcu.column_name FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
        """)
        return [row['column_name'] for row in pk_res]
    ```

3.  **Запрос для извлечения всех данных для NoSQL-сериализации**
    ```sql
    SELECT * FROM public."table_name";
    ```
    **Реализация:**
    ```python
    def fetch_all_data(table_name):
        return execute_query(f'SELECT * FROM public."{table_name}"')
    ```

4.  **Аналитический запрос: Статистика по категориям годности**
    ```sql
    SELECT category_id, COUNT(*) as conscript_count 
    FROM medical_examinations 
    GROUP BY category_id ORDER BY conscript_count DESC;
    ```

5.  **Комплексный запрос: Данные призывника с военным билетом и учетной картой**
    ```sql
    SELECT c.full_name, m.ticket_number, s.card_number 
    FROM conscripts c 
    LEFT JOIN military_id_cards m ON c.military_ticket_id = m.ticket_id 
    LEFT JOIN service_record_cards s ON c.registration_card_id = s.card_id;
    ```

---

<div id="разработка-приложения"></div>

### 2 РАЗРАБОТКА ПРИЛОЖЕНИЯ

#### 2.1 Логическая и физическая структура NoSQL

В рамках реализации проекта была спроектирована и разработана специализированная NoSQL база данных на основе встраиваемой архитектуры Berkeley DB (реализованная средствами интерфейса `dbm`).

**Логическая организация данных:**
Для каждой таблицы исходной реляционной базы данных создается именованное хранилище. 
*   **Ключ (Key):** Строковое представление первичного ключа. Для таблиц связей (Many-to-Many), таких как `conscripts_events`, используется составной ключ формата `ID_ID` (например, `1_5`).
*   **Значение (Value):** Сериализованный объект в формате JSON. Это позволяет хранить всю информацию о записи в одном значении NoSQL базы, инкапсулируя разнородные атрибуты без потери их семантики.

**Физическая структура (Специфика реализации на Windows):**
При использовании библиотеки `dbm` в среде Windows (интерфейс `dbm.dumb`), физическое хранение каждой логической таблицы осуществляется через набор файлов в директории `nosql_db_python`:
*   `.dat` — файл, содержащий непосредственно бинарные данные.
*   `.dir` — индексный файл, обеспечивающий быстрый поиск по ключу.
*   `.bak` — резервная копия индекса.
*   `.txt` — текстовый дамп (экспорт), предназначенный для верификации данных человеком без использования специальных утилит.

Такой подход обеспечивает высокую скорость доступа к данным по ключу (O(1)) и позволяет избежать избыточности, характерной для классических JOIN-запросов в РСУБД.

#### 2.2 Верификация данных (Инспекция NoSQL)

Для подтверждения корректности записи в бинарные файлы BerkeleyDB было разработано утилитарное средство инспекции, которое открывает `.db` файлы напрямую через библиотеку `dbm`.

**Листинг 2.1 – Пример вывода инспектора бинарных файлов**

```text
Проверка базы: conscripts.db
Ключ: 1 | Значение: {"full_name": "Александров Алексей Сергеевич", ...}
Ключ: 24 | Значение: {"full_name": "Мистер Подхвост", ...}

Проверка базы: conscripts_events.db
Ключ: 1_5 | Значение: {}
```

---

<div id="заключение"></div>

### ЗАКЛЮЧЕНИЕ

В результате выполнения лабораторной работы был успешно реализован конвертор данных из реляционной СУБД PostgreSQL в NoSQL хранилище BerkeleyDB (Key-Value). 

В ходе работы были решены следующие задачи:
1.  Разработан алгоритм динамического маппинга схем, позволяющий работать с любой структурой таблиц без ручного описания полей.
2.  Реализована обработка составных первичных ключей для корректного переноса связей «многие-ко-многим».
3.  Разработана утилита-инспектор (`inspector.py`) для прямого доступа к бинарным данным BerkeleyDB в обход текстовых дампов.

Особенностью решения стало использование JSON-сериализации для хранения сложных объектов, что обеспечивает гибкость системы при изменении структуры данных. Использование BerkeleyDB позволило значительно упростить архитектуру приложения в части доступа к данным по уникальным идентификаторам.

---

<div id="приложение-а"></div>

### ПРИЛОЖЕНИЕ А. Листинг кода

**Файл converter.py**

```python
import json
import os
import dbm
from datetime import datetime, date, time
from decimal import Decimal

try:
    from database import execute_query
except ImportError:
    print("Ошибка: Не найден файл database.py")
    exit(1)

class NoSQLJSONEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (datetime, date, time)):
            return obj.isoformat()
        elif isinstance(obj, Decimal):
            return float(obj)
        return super().default(obj)

def convert():
    nosql_dir = os.path.join(os.path.dirname(__file__), "..", "nosql_db_python")
    os.makedirs(nosql_dir, exist_ok=True)

    tables_res = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'")
    tables = [row['table_name'] for row in tables_res]

    for table_name in tables:
        pk_res = execute_query(f"SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}' ORDER BY kcu.ordinal_position")
        pk_cols = [row['column_name'] for row in pk_res]
        
        data = execute_query(f'SELECT * FROM public."{table_name}"')
        
        db_path = os.path.join(nosql_dir, f"{table_name}.db")
        with dbm.open(db_path, 'n') as db:
            for row in data:
                if len(pk_cols) > 1:
                    key_str = "_".join([str(row[c]) for c in pk_cols])
                elif len(pk_cols) == 1:
                    key_str = str(row[pk_cols[0]])
                else:
                    key_str = str(list(row.values())[0])

                val_dict = {k: v for k, v in row.items() if k not in pk_cols}
                db[key_str] = json.dumps(val_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)

if __name__ == "__main__":
    convert()
```
