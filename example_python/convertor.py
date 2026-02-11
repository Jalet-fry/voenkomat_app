#converter.py
import psycopg2
import json
import os
import subprocess
from datetime import datetime, date, time
from decimal import Decimal

# Настройки базы данных
DB_PARAMS = {
    'dbname': 'school_db',
    'user': 'postgres',
    'password': '123',
    'host': 'localhost',
    'port': '5432'
}

# Путь к утилите dbsql
DBSQL_PATH = r"C:\Program Files (x86)\Oracle\Berkeley DB 12cR1 6.0.30\bin\dbsql.exe"
BERKELEY_DIR = "./berkeley_db"


class JSONEncoder(json.JSONEncoder):
    """Кастомный JSON encoder для обработки специальных типов данных"""

    def default(self, obj):
        if isinstance(obj, (datetime, date)):
            return obj.isoformat()
        elif isinstance(obj, time):
            return obj.isoformat()
        elif isinstance(obj, Decimal):
            return float(obj)
        elif isinstance(obj, bytes):
            return obj.decode('utf-8', errors='ignore')
        return super().default(obj)


class PostgresToBerkeleyConverter:
    def __init__(self):
        self.pg_conn = None
        self.berkeley_dir = BERKELEY_DIR

    def connect_postgres(self):
        """Подключение к PostgreSQL"""
        try:
            self.pg_conn = psycopg2.connect(**DB_PARAMS)
            return True, "Успешное подключение к PostgreSQL"
        except Exception as e:
            return False, f"Ошибка подключения к PostgreSQL: {e}"

    def get_table_names(self):
        """Получает список всех таблиц"""
        query = """
        SELECT table_name
        FROM information_schema.tables
        WHERE table_schema = 'public'
        """
        with self.pg_conn.cursor() as cursor:
            cursor.execute(query)
            return [row[0] for row in cursor.fetchall()]

    def get_table_columns(self, table_name):
        """Получает столбцы таблицы"""
        query = """
        SELECT column_name
        FROM information_schema.columns
        WHERE table_name = %s
        ORDER BY ordinal_position
        """
        with self.pg_conn.cursor() as cursor:
            cursor.execute(query, (table_name,))
            return [row[0] for row in cursor.fetchall()]

    def get_primary_keys(self, table_name):
        """Получает первичные ключи таблицы"""
        query = """
        SELECT kcu.column_name
        FROM information_schema.table_constraints tc
        JOIN information_schema.key_column_usage kcu 
            ON tc.constraint_name = kcu.constraint_name
        WHERE tc.table_name = %s 
            AND tc.constraint_type = 'PRIMARY KEY'
        ORDER BY kcu.ordinal_position
        """
        with self.pg_conn.cursor() as cursor:
            cursor.execute(query, (table_name,))
            result = cursor.fetchall()
            if result:
                return [row[0] for row in result]
            else:
                # Если первичный ключ не найден, проверяем наличие поля 'id'
                columns = self.get_table_columns(table_name)
                if 'id' in columns:
                    return ['id']
                else:
                    # Если нет 'id', используем первый столбец
                    return [columns[0]] if columns else []

    def get_table_data(self, table_name, columns):
        """Получает данные таблицы с сортировкой по ID для consistency"""
        columns_str = ', '.join(columns)

        # Добавляем ORDER BY если есть поле id
        order_by = ""
        if 'id' in columns:
            order_by = " ORDER BY id"
        elif columns:
            order_by = f" ORDER BY {columns[0]}"

        query = f"SELECT {columns_str} FROM {table_name}{order_by}"

        with self.pg_conn.cursor() as cursor:
            cursor.execute(query)
            return cursor.fetchall()


    def create_berkeley_database_dir(self):
        """Создает директорию для баз данных Berkeley"""
        if not os.path.exists(self.berkeley_dir):
            os.makedirs(self.berkeley_dir)
            print(f"Создана директория для Berkeley DB: {self.berkeley_dir}")

    def convert_value(self, value):
        """Конвертирует значение в JSON-совместимый формат"""
        if value is None:
            return None
        elif isinstance(value, (datetime, date)):
            return value.isoformat()
        elif isinstance(value, time):
            return value.isoformat()
        elif isinstance(value, Decimal):
            return float(value)
        elif isinstance(value, (int, float, str, bool)):
            return value
        elif isinstance(value, bytes):
            return value.decode('utf-8', errors='ignore')
        else:
            return str(value)

    def get_primary_key_value(self, table_name, row, columns, primary_keys):
        """Формирует значение первичного ключа для Berkeley DB"""
        if len(primary_keys) == 1:
            key_index = columns.index(primary_keys[0])
            return str(self.convert_value(row[key_index]))
        else:
            # Для составных ключей (например, order_products)
            key_parts = []
            for pk_column in primary_keys:
                key_index = columns.index(pk_column)
                key_parts.append(str(self.convert_value(row[key_index])))
            return '_'.join(key_parts)

    def convert_table(self, table_name, progress_callback=None):
        """Конвертирует одну таблицу в Berkeley DB"""
        try:
            # Получаем структуру таблицы
            columns = self.get_table_columns(table_name)
            primary_keys = self.get_primary_keys(table_name)

            print(f"Конвертация таблицы: {table_name}")
            print(f"Столбцы: {columns}")
            print(f"Первичные ключи: {primary_keys}")

            # Получаем данные
            data = self.get_table_data(table_name, columns)
            print(f"Найдено записей: {len(data)}")

            # Создаем SQL скрипт для Berkeley DB
            sql_script = f"CREATE TABLE {table_name} (key STRING PRIMARY KEY, value STRING);\n"

            records_count = 0
            total_records = len(data)

            for i, row in enumerate(data):
                try:
                    # Формируем ключ
                    key = self.get_primary_key_value(table_name, row, columns, primary_keys)

                    # Создаем JSON объект
                    row_dict = {}
                    for j, column in enumerate(columns):
                        row_dict[column] = self.convert_value(row[j])

                    # Сериализуем в JSON
                    value_json = json.dumps(row_dict, ensure_ascii=False, cls=JSONEncoder)

                    # Экранируем кавычки для SQL
                    value_escaped = value_json.replace("'", "''")

                    # Добавляем INSERT команду
                    sql_script += f"INSERT INTO {table_name} VALUES ('{key}', '{value_escaped}');\n"
                    records_count += 1

                    # Обновляем прогресс
                    if progress_callback and total_records > 0:
                        progress = int((i + 1) / total_records * 100)
                        progress_callback(progress, f"Обработано {i + 1} из {total_records} записей")

                except Exception as e:
                    print(f"Ошибка при конвертации записи в таблице {table_name}: {e}")
                    continue

            # Создаем временный файл с SQL командами
            temp_sql_file = os.path.join(self.berkeley_dir, f"temp_{table_name}.sql")
            with open(temp_sql_file, 'w', encoding='utf-8') as f:
                f.write(sql_script)

            # Создаем Berkeley DB через dbsql
            db_path = os.path.join(self.berkeley_dir, f"{table_name}.db")

            # Выполняем команду dbsql с перенаправлением ввода с правильной кодировкой
            cmd = f'"{DBSQL_PATH}" "{db_path}" < "{temp_sql_file}"'
            print(f"Выполняем команду: {cmd}")

            # Используем UTF-8 кодировку для избежания проблем с Unicode
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, encoding='utf-8', errors='ignore')

            print(f"Код возврата: {result.returncode}")
            if result.stdout:
                print(f"Вывод: {result.stdout}")
            if result.stderr:
                print(f"Ошибки: {result.stderr}")

            # Удаляем временный файл
            if os.path.exists(temp_sql_file):
                os.remove(temp_sql_file)

            if result.returncode == 0:
                return True, f"Таблица {table_name} сконвертирована успешно ({records_count} записей)"
            else:
                return False, f"Ошибка создания Berkeley DB для {table_name}: {result.stderr}"

        except Exception as e:
            return False, f"Ошибка при конвертации таблицы {table_name}: {e}"

    # В converter.py добавим метод clear_berkeley_dir и изменим convert_all_tables

    def clear_berkeley_dir(self):
        """Очищает папку с BerkeleyDB файлами"""
        if os.path.exists(self.berkeley_dir):
            for filename in os.listdir(self.berkeley_dir):
                file_path = os.path.join(self.berkeley_dir, filename)
                try:
                    if os.path.isfile(file_path) and filename.endswith(('.db', '.sql')):
                        os.remove(file_path)
                        print(f"Удален файл: {filename}")
                except Exception as e:
                    print(f"Ошибка при удалении файла {filename}: {e}")

    def convert_all_tables(self, progress_callback=None):
        """Конвертирует все таблицы"""
        try:
            self.create_berkeley_database_dir()

            # Очищаем папку перед началом конвертации
            self.clear_berkeley_dir()

            table_names = self.get_table_names()
            total_tables = len(table_names)

            print(f"Найдены таблицы: {table_names}")

            successful_tables = []
            failed_tables = []

            for i, table_name in enumerate(table_names):
                if progress_callback:
                    progress = int((i + 1) / total_tables * 100)
                    progress_callback(progress, f"Конвертация таблицы: {table_name}")

                success, message = self.convert_table(table_name, progress_callback)
                if success:
                    successful_tables.append(table_name)
                else:
                    failed_tables.append((table_name, message))

            return successful_tables, failed_tables

        except Exception as e:
            return [], [("Все таблицы", f"Критическая ошибка: {e}")]

    def close_connections(self):
        """Закрывает соединения"""
        if self.pg_conn:
            self.pg_conn.close()


def main():
    """Запуск конвертации из командной строки"""
    # Проверяем наличие dbsql
    if not os.path.exists(DBSQL_PATH):
        print(f"ОШИБКА: Не найден dbsql.exe по пути: {DBSQL_PATH}")
        print("Убедитесь, что Berkeley DB установлена правильно")
        return

    converter = PostgresToBerkeleyConverter()

    try:
        # Подключаемся к PostgreSQL
        success, message = converter.connect_postgres()
        if not success:
            print(f"Ошибка: {message}")
            return

        print("Начинаем конвертацию PostgreSQL → BerkeleyDB...")

        # Запускаем конвертацию
        successful_tables, failed_tables = converter.convert_all_tables()

        # Выводим результаты
        print("\n" + "=" * 50)
        print("РЕЗУЛЬТАТЫ КОНВЕРТАЦИИ:")
        print(f"Успешно: {len(successful_tables)} таблиц")
        print(f"С ошибками: {len(failed_tables)} таблиц")

        if successful_tables:
            print("Успешные таблицы:", successful_tables)
        if failed_tables:
            print("Таблицы с ошибками:")
            for table_name, error in failed_tables:
                print(f"  • {table_name}: {error}")

        print(f"\nБазы данных сохранены в папке: {BERKELEY_DIR}")

    except Exception as e:
        print(f"Критическая ошибка при конвертации: {e}")

    finally:
        converter.close_connections()


if __name__ == "__main__":
    main()