import sqlite3
import json
import os
from datetime import datetime, date, time
from decimal import Decimal

# Импортируем вашу функцию для работы с Postgres
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
    print("\n--- КОНВЕРТАЦИЯ В NoSQL (Один файл .db на таблицу) ---")
    
    nosql_dir = os.path.join(os.path.dirname(__file__), "..", "nosql_db_python")
    if not os.path.exists(nosql_dir):
        os.makedirs(nosql_dir)

    # 1. Получение списка таблиц из Postgres
    try:
        tables_res = execute_query("""
            SELECT table_name 
            FROM information_schema.tables 
            WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
        """)
        tables = [row['table_name'] for row in tables_res]
    except Exception as e:
        print(f"Ошибка подключения к Postgres: {e}")
        return

    for table_name in tables:
        print(f"Обработка {table_name}...")
        
        # Получаем структуру и данные
        try:
            # Выясняем PK
            pk_res = execute_query(f"""
                SELECT kcu.column_name 
                FROM information_schema.table_constraints tc 
                JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
                WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
            """)
            pk_cols = [row['column_name'] for row in pk_res] if pk_res else []
            
            data = execute_query(f'SELECT * FROM public."{table_name}"')
            
            # Файл базы данных (теперь строго один .db)
            db_path = os.path.join(nosql_dir, f"{table_name}.db")
            if os.path.exists(db_path): os.remove(db_path) # Очищаем старый

            # Подключаемся через sqlite3 (имитируем Berkeley DB SQL)
            conn = sqlite3.connect(db_path)
            conn.execute("CREATE TABLE kv (key TEXT PRIMARY KEY, value TEXT)")

            for row in data:
                # Формируем ключ
                key_str = "_".join([str(row[c]) for c in pk_cols]) if pk_cols else str(list(row.values())[0])
                
                # ФОРМИРОВАНИЕ ЗНАЧЕНИЯ (JSON)
                # Теперь сохраняем ВСЕ колонки в JSON, чтобы в UI ничего не пропадало
                val_dict = {k: v for k, v in row.items()}
                json_val = json.dumps(val_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)
                
                conn.execute("INSERT INTO kv (key, value) VALUES (?, ?)", (key_str, json_val))
            
            conn.commit()
            conn.close()
            print(f"  OK: Создан файл {table_name}.db")
            
        except Exception as e:
            print(f"  Ошибка в {table_name}: {e}")

    print("\n--- КОНВЕРТАЦИЯ ЗАВЕРШЕНА. В папке nosql_db_python теперь только .db файлы ---")

if __name__ == "__main__":
    convert()
