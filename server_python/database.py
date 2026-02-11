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
        "user": "postgres",
        "password": "" 
    }
    
    if os.path.exists(config_path):
        config.read(config_path, encoding="utf-8")
        if "Database" in config:
            raw_config = dict(config["Database"])
            if "username" in raw_config:
                db_params["user"] = raw_config.pop("username")
            db_params.update(raw_config)

    env_password = os.getenv("PGPASSWORD")
    if env_password:
        db_params["password"] = env_password
        
    return db_params

def get_db_connection():
    db_params = get_db_config()
    if not db_params.get("password"):
        raise HTTPException(status_code=500, detail="Database password not found.")
        
    try:
        valid_keys = ['host', 'port', 'database', 'user', 'password']
        connect_params = {k: v for k, v in db_params.items() if k in valid_keys}
        return psycopg2.connect(**connect_params)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

def execute_query(query, params=None):
    conn = get_db_connection()
    cursor = conn.cursor(cursor_factory=RealDictCursor)
    try:
        cursor.execute(query, params)
        if query.strip().upper().startswith('SELECT'):
            result = cursor.fetchall()
        else:
            conn.commit()
            result = {"rows_affected": cursor.rowcount}
        return result
    finally:
        cursor.close()
        conn.close()
