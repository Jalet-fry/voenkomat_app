import json
import os
import dbm
from datetime import datetime, date, time
from decimal import Decimal

# TODO: [LAB3] Реализация NoSQL конвертера (Postgres -> BerkeleyDB/dbm)
# Соответствует требованиям спецификации: Ключ=PK, Значение=JSON.

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
    print("--- ЛАБОРАТОРНАЯ РАБОТА №3: КОНВЕРТАЦИЯ В NoSQL ---")
    
    # [ШАГ 1] Подключение к БД осуществляется внутри execute_query
    nosql_dir = "nosql_db"
    if not os.path.exists(nosql_dir):
        os.makedirs(nosql_dir)

    # [ШАГ 2] Получение информации о таблицах (названия, столбцы, данные)
    tables_res = execute_query("""
        SELECT table_name 
        FROM information_schema.tables 
        WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
    """)
    tables = [row['table_name'] for row in tables_res]

    for table_name in tables:
        print(f"Конвертация {table_name}...")
        
        # Получаем структуру PK для формирования ключа
        pk_query = f"""
            SELECT kcu.column_name 
            FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
            ORDER BY kcu.ordinal_position
        """
        pk_cols = [row['column_name'] for row in pk_res] if (pk_res := execute_query(pk_query)) else []

        # Получаем содержимое таблицы
        data = execute_query(f"SELECT * FROM public.{table_name}")
        db_path = os.path.join(nosql_dir, f"{table_name}.db")
        
        # [ШАГ 3] Создание баз данных BerkeleyDB (dbm) и их заполнение
        try:
            with dbm.open(db_path, 'n') as db:
                for row in data:
                    # ФОРМИРОВАНИЕ КЛЮЧА (согласно Таблице 2.1 ТЗ)
                    if len(pk_cols) > 1:
                        # Для связей M2M ключ вида {id1}_{id2}
                        key_str = "_".join([str(row[c]) for c in pk_cols])
                    elif len(pk_cols) == 1:
                        # Для обычных таблиц ключ - это ID
                        key_str = str(row[pk_cols[0]])
                    else:
                        key_str = str(list(row.values())[0])

                    # ФОРМИРОВАНИЕ ЗНАЧЕНИЯ (JSON)
                    # Если PK один, исключаем его из JSON (как в примере students: id -> {...})
                    if len(pk_cols) == 1:
                        val_dict = {k: v for k, v in row.items() if k not in pk_cols}
                    else:
                        val_dict = row

                    db[key_str] = json.dumps(val_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)
                
                print(f"  OK: {table_name}.db создана.")
        except Exception as e:
            print(f"  Ошибка: {e}")

    # [ШАГ 4] Закрытие соединений происходит автоматически при выходе из with и функций
    print("\n--- КОНВЕРТАЦИЯ ЗАВЕРШЕНА (TODO: [LAB3] OK) ---")

if __name__ == "__main__":
    convert()
