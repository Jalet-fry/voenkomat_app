import json
import os
try:
    from bsddb3 import db
except ImportError:
    try:
        from berkeleydb import db
    except ImportError:
        print("Ошибка: Не установлена библиотека BerkeleyDB (bsddb3 или berkeleydb).")
        print("Установите её командой: pip install bsddb3")
        exit(1)

from database import execute_query, get_db_connection

def convert():
    print("--- Запуск конвертации PostgreSQL -> BerkeleyDB ---")
    
    # 1. Получаем список всех таблиц в схеме public
    tables_query = "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'"
    tables_res = execute_query(tables_query)
    tables = [row['table_name'] for row in tables_res]
    
    if not tables:
        print("Таблиц в базе данных не найдено.")
        return

    # Создаем папку для NoSQL баз, если её нет
    if not os.path.exists("nosql_db"):
        os.makedirs("nosql_db")

    for table_name in tables:
        print(f"Обработка таблицы: {table_name}...")
        
        # 2. Получаем названия столбцов
        cols_query = f"SELECT column_name FROM information_schema.columns WHERE table_name = '{table_name}' ORDER BY ordinal_position"
        cols_res = execute_query(cols_query)
        columns = [row['column_name'] for row in cols_res]
        
        # Находим первичный ключ (для формирования ключа в BDB)
        pk_query = f"""
            SELECT kcu.column_name 
            FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
        """
        pk_res = execute_query(pk_query)
        pk_cols = [row['column_name'] for row in pk_res]
        
        # 3. Получаем данные
        data = execute_query(f"SELECT * FROM {table_name}")
        
        # 4. Создаем базу BerkeleyDB
        db_path = os.path.join("nosql_db", f"{table_name}.db")
        bdb = db.DB()
        bdb.open(db_path, None, db.DB_HASH, db.DB_CREATE)

        count = 0
        for row in data:
            # Формируем КЛЮЧ
            if len(pk_cols) == 1:
                # Обычный ключ (ID)
                key = str(row[pk_cols[0]])
            elif len(pk_cols) > 1:
                # Составной ключ (для Many-to-Many) типа {id1}_{id2}
                key = "_".join([str(row[c]) for c in pk_cols])
            else:
                # Если ПК нет (не должно быть по схеме), используем первую колонку
                key = str(row[columns[0]])

            # Формируем ЗНАЧЕНИЕ (JSON без полей первичного ключа, как в ТЗ)
            value_dict = {k: str(v) if v is not None else None for k, v in row.items() if k not in pk_cols}
            value_json = json.dumps(value_dict, ensure_ascii=False)

            # Записываем в BerkeleyDB (нужны байты)
            bdb.put(key.encode('utf-8'), value_json.encode('utf-8'))
            count += 1

        bdb.close()
        print(f"  Успешно: {count} записей перенесено в {db_path}")

    print("\n--- Конвертация полностью завершена! ---")

if __name__ == "__main__":
    convert()
