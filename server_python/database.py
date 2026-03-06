import psycopg2
from psycopg2.extras import RealDictCursor
import os
import configparser
from fastapi import HTTPException

def get_db_config():
    config = configparser.ConfigParser()
    config_path = os.path.join(os.path.dirname(__file__), "..", "config.ini")
    db_params = {"host": "localhost", "port": "5432", "database": "military_db", "user": "postgres"}
    if os.path.exists(config_path):
        try:
            config.read(config_path, encoding="utf-8")
            if "Database" in config:
                raw_config = dict(config["Database"])
                if "username" in raw_config: db_params["user"] = raw_config.pop("username")
                if "password" in raw_config and raw_config["password"]: db_params["password"] = raw_config["password"]
                for k, v in raw_config.items():
                    if k != "password": db_params[k] = v
        except: pass
    return db_params

def get_db_connection():
    db_params = get_db_config()
    try:
        valid_keys = ['host', 'port', 'database', 'user', 'password']
        connect_params = {k: v for k, v in db_params.items() if k in valid_keys and v}
        return psycopg2.connect(**connect_params)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Ошибка подключения к БД: {str(e)}")

def execute_query(query, params=None):
    conn = None
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        cursor.execute(query, params)

        result = None
        if cursor.description:
            # Если есть результат (SELECT или INSERT...RETURNING)
            result = cursor.fetchall()
            # ВАЖНО: всегда коммитим, так как это может быть INSERT/UPDATE с RETURNING
            conn.commit()
        else:
            # Если это обычный UPDATE/DELETE без возврата данных
            conn.commit()
            result = [{"rows_affected": cursor.rowcount}]
            
        cursor.close()
        return result
    except Exception as e:
        if conn: conn.rollback()
        print(f"SQL Error: {e}")
        raise HTTPException(status_code=400, detail=str(e))
    finally:
        if conn: conn.close()
