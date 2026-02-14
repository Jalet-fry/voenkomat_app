import psycopg2
from psycopg2.extras import RealDictCursor
import os
import configparser
from fastapi import HTTPException

def get_db_config():
    config = configparser.ConfigParser()
    config_path = os.path.join(os.path.dirname(__file__), "..", "config.ini")
    
    db_params = {
        "host": "localhost",
        "port": "5432",
        "database": "military_db",
        "user": "postgres"
    }
    
    # 1. Читаем из конфига (кроме пароля, если его там нет)
    if os.path.exists(config_path):
        try:
            config.read(config_path, encoding="utf-8")
            if "Database" in config:
                raw_config = dict(config["Database"])
                if "username" in raw_config:
                    db_params["user"] = raw_config.pop("username")
                # Пароль берем из конфига только если он там явно прописан
                if "password" in raw_config and raw_config["password"]:
                    db_params["password"] = raw_config["password"]

                # Обновляем остальные параметры (host, port и т.д.)
                for k, v in raw_config.items():
                    if k != "password":
                        db_params[k] = v
        except Exception as e:
            print(f"Warning: Could not read config file: {e}")

    # 2. Проверяем переменную окружения PGPASSWORD
    env_password = os.getenv("PGPASSWORD")
    if env_password:
        db_params["password"] = env_password

    return db_params

def get_db_connection():
    db_params = get_db_config()

    try:
        # Мы передаем только те параметры, которые у нас есть.
        # Если пароля нет в словаре, psycopg2 (через libpq) сам будет искать его в:
        # 1. Переменной окружения PGPASSWORD
        # 2. Файле %APPDATA%\postgresql\pgpass.conf (стандарт для Windows)
        # Это именно то, как работает Qt и почему он "спокойно находит пароль".

        valid_keys = ['host', 'port', 'database', 'user', 'password']
        connect_params = {k: v for k, v in db_params.items() if k in valid_keys and v}

        return psycopg2.connect(**connect_params)
    except Exception as e:
        print(f"DB Connection Error: {e}")
        # Если не удалось найти пароль даже системно
        if "no password supplied" in str(e).lower():
            raise HTTPException(status_code=500, detail="Пароль базы данных не найден ни в переменных, ни в системе.")
        raise HTTPException(status_code=500, detail=f"Ошибка подключения к БД: {str(e)}")

def execute_query(query, params=None):
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        cursor.execute(query, params)

        if query.strip().upper().startswith('SELECT'):
            result = cursor.fetchall()
        else:
            conn.commit()
            result = {"rows_affected": cursor.rowcount}
        cursor.close()
        return result
    except Exception as e:
        if conn:
            conn.rollback()
        print(f"Query Error: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        if conn:
            conn.close()
