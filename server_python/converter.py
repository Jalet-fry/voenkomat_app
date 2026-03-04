# TODO: [REVIEW] OK.
# TODO: [NOTE] Implementation uses JSON for values in BerkeleyDB (Key-Value Core API).
# TODO: [FIX] Added automatic cleanup of old database files and advanced JSON encoding.

import json
import os
import shutil
from datetime import datetime, date, time
from decimal import Decimal

try:
    from bsddb3 import db
except ImportError:
    try:
        from berkeleydb import db
    except ImportError:
        print("Ошибка: Не установлена библиотека BerkeleyDB (bsddb3 или berkeleydb).")
        print("Установите её командой: pip install bsddb3")
        exit(1)

from database import execute_query

class NoSQLJSONEncoder(json.JSONEncoder):
    """Кастомный JSON encoder для корректного переноса типов данных из PostgreSQL"""
    def default(self, obj):
        if isinstance(obj, (datetime, date, time)):
            return obj.isoformat()
        elif isinstance(obj, Decimal):
            return float(obj)
        elif isinstance(obj, bytes):
            return obj.decode('utf-8', errors='ignore')
        return super().default(obj)

def convert():
    print("--- Запуск конвертации PostgreSQL -> BerkeleyDB (NoSQL) ---")
    
    nosql_dir = "nosql_db"
    
    # 1. Очистка и создание папки
    if os.path.exists(nosql_dir):
        print(f"Очистка старых данных в {nosql_dir}...")
        for file in os.listdir(nosql_dir):
            if file.endswith(".db"):
                os.remove(os.path.join(nosql_dir, file))
    else:
        os.makedirs(nosql_dir)

    # 2. Получаем список всех таблиц
    tables_query = "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'"
    tables_res = execute_query(tables_query)
    tables = [row['table_name'] for row in tables_res]
    
    if not tables:
        print("Таблиц в базе данных не найдено.")
        return

    for table_name in tables:
        print(f"Обработка таблицы: {table_name}...")
        
        # 3. Находим первичный ключ для формирования ключа BerkeleyDB
        pk_query = f"""
            SELECT kcu.column_name 
            FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
            ORDER BY kcu.ordinal_position
        """
        pk_res = execute_query(pk_query)
        pk_cols = [row['column_name'] for row in pk_res]
        
        # 4. Получаем данные
        data = execute_query(f"SELECT * FROM public.{table_name}")
        
        # 5. Создаем базу BerkeleyDB (HASH-таблица)
        db_path = os.path.join(nosql_dir, f"{table_name}.db")
        bdb = db.DB()
        bdb.open(db_path, None, db.DB_HASH, db.DB_CREATE)

        count = 0
        for row in data:
            # Формируем КЛЮЧ (ID или составной)
            if pk_cols:
                key = "_".join([str(row[c]) for c in pk_cols])
            else:
                # Если ПК нет (не по схеме), используем первый столбец
                key = str(next(iter(row.values())))

            # Формируем ЗНАЧЕНИЕ (JSON без полей первичного ключа)
            # Мы используем кастомный энкодер для дат и чисел
            value_dict = {k: v for k, v in row.items() if k not in pk_cols}
            value_json = json.dumps(value_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)

            # Записываем байты
            bdb.put(key.encode('utf-8'), value_json.encode('utf-8'))
            count += 1

        bdb.close()
        print(f"  Успешно: {count} записей перенесено в {db_path}")

    print("\n--- Конвертация завершена! Данные сохранены в Key-Value формате. ---")

if __name__ == "__main__":
    convert()
