import json
import os
import dbm  # Встроенная библиотека Python, аналог BerkeleyDB (Key-Value), работает без установки pip
from datetime import datetime, date, time
from decimal import Decimal

# Импортируем менеджер базы данных из вашего проекта
try:
    from database import execute_query
except ImportError:
    print("Ошибка: Не найден файл database.py в текущей директории.")
    exit(1)

class NoSQLJSONEncoder(json.JSONEncoder):
    """Кастомный энкодер для типов данных Postgres (ISO даты, Decimal в float)"""
    def default(self, obj):
        if isinstance(obj, (datetime, date, time)):
            return obj.isoformat()
        elif isinstance(obj, Decimal):
            return float(obj)
        elif isinstance(obj, bytes):
            return obj.decode('utf-8', errors='ignore')
        return super().default(obj)

def convert():
    print("--- ЛАБОРАТОРНАЯ РАБОТА №3: КОНВЕРТАЦИЯ В NoSQL ---")
    
    nosql_dir = "nosql_db"
    if not os.path.exists(nosql_dir):
        os.makedirs(nosql_dir)
        print(f"Создана директория: {nosql_dir}")
    else:
        # Очистка старых файлов (согласно ТЗ: приложение создает базу заново)
        print(f"Очистка старых данных в {nosql_dir}...")
        for f in os.listdir(nosql_dir):
            try:
                os.remove(os.path.join(nosql_dir, f))
            except:
                pass

    # 1. Получаем список всех таблиц схемы public (согласно алгоритму ТЗ)
    tables_res = execute_query("""
        SELECT table_name 
        FROM information_schema.tables 
        WHERE table_schema = 'public' AND table_type = 'BASE TABLE'
    """)
    tables = [row['table_name'] for row in tables_res]

    if not tables:
        print("Таблиц в базе данных PostgreSQL не найдено.")
        return

    for table_name in tables:
        print(f"\nОбработка таблицы: {table_name}...")
        
        # 2. Получаем первичные ключи для формирования ключа BerkeleyDB (dbm)
        pk_query = f"""
            SELECT kcu.column_name 
            FROM information_schema.table_constraints tc 
            JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name 
            WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'
            ORDER BY kcu.ordinal_position
        """
        pk_res = execute_query(pk_query)
        pk_cols = [row['column_name'] for row in pk_res]

        # 3. Получаем данные таблицы
        data = execute_query(f"SELECT * FROM public.{table_name}")
        
        # Путь к файлу NoSQL базы
        db_path = os.path.join(nosql_dir, f"{table_name}.db")
        
        # 4. Создаем базу BerkeleyDB (через dbm) и заполняем ее
        try:
            # 'n' - create new, 'c' - read/write (create if not exists)
            # dbm на Windows создаст файлы .dat и .dir
            with dbm.open(db_path, 'n') as db:
                count = 0
                for row in data:
                    # ФОРМИРУЕМ КЛЮЧ (Key)
                    if pk_cols:
                        # Если ключ составной (M2M), соединяем через '_' как в ТЗ (напр. 1_5)
                        key_str = "_".join([str(row[c]) for c in pk_cols])
                    else:
                        # Если ПК нет, используем значение первой колонки
                        key_str = str(next(iter(row.values())))

                    # ФОРМИРУЕМ ЗНАЧЕНИЕ (Value в формате JSON)
                    # Согласно ТЗ (таблица 2.1), в значении храним столбцы за вычетом PK (если PK одиночный)
                    # Но для составных ключей в ТЗ пример показывает наличие полей в JSON.
                    # Сделаем универсально: исключаем PK только если он один.
                    if len(pk_cols) == 1:
                        value_dict = {k: v for k, v in row.items() if k not in pk_cols}
                    else:
                        value_dict = row

                    value_json = json.dumps(value_dict, ensure_ascii=False, cls=NoSQLJSONEncoder)

                    # Записываем в хранилище (dbm требует строки или байты)
                    db[key_str] = value_json
                    count += 1
                
                print(f"  Успешно: {count} записей перенесено в {table_name}.db")
        except Exception as e:
            print(f"  ОШИБКА при создании базы {table_name}: {e}")

    print("\n--- КОНВЕРТАЦИЯ ПОЛНОСТЬЮ ЗАВЕРШЕНА ---")
    print(f"Результаты сохранены в папке: {os.path.abspath(nosql_dir)}")

if __name__ == "__main__":
    convert()
