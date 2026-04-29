import sqlite3
import json
import os
import re
from typing import List, Dict, Any, Optional

# Важно: используем ту же папку, что и C++ сервер для совместимости
NOSQL_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "nosql_db_cpp"))

class NoSQLManager:
    def __init__(self):
        if not os.path.exists(NOSQL_DIR):
            os.makedirs(NOSQL_DIR)

    def _get_db_path(self, table_name: str) -> str:
        return os.path.join(NOSQL_DIR, f"{table_name}.db")

    def _get_connection(self, table_name: str):
        db_path = self._get_db_path(table_name)
        conn = sqlite3.connect(db_path)
        conn.row_factory = sqlite3.Row
        conn.execute("CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT)")
        return conn

    def list_tables(self) -> List[str]:
        if not os.path.exists(NOSQL_DIR): return []
        return sorted([f.replace('.db', '') for f in os.listdir(NOSQL_DIR) if f.endswith('.db')])

    def get_all_data(self, table_name: str) -> List[Dict[str, Any]]:
        return self.execute_filter(table_name, None)

    def execute_filter(self, table_name: str, filter_str: Optional[str] = None) -> List[Dict[str, Any]]:
        if not os.path.exists(self._get_db_path(table_name)): return []
        
        sql = "SELECT key, value FROM kv"
        params = []
        
        if filter_str:
            # Парсинг фильтра: col op val
            match = re.search(r"(\w+)\s*(=|>|<|>=|<=|ILIKE)\s*(.+)", filter_str, re.IGNORECASE)
            if match:
                col, op, val = match.groups()
                val = val.strip("'\"% ")
                
                # Если фильтр по виртуальному ID (__pk)
                target_col = "key" if col == "__pk" else f"json_extract(value, '$.{col}')"
                
                if op.upper() == "ILIKE":
                    sql += f" WHERE {target_col} LIKE ?"
                    params.append(f"%{val}%")
                else:
                    # Умное сравнение для ключей (длина, значение) или CAST для чисел
                    if col == "__pk":
                        sql += f" WHERE (length(key), key) {op} (length(?), ?)"
                        params.extend([val, val])
                    else:
                        # Проверка на число
                        if val.replace('.','',1).isdigit():
                            sql += f" WHERE CAST({target_col} AS REAL) {op} ?"
                            params.append(float(val))
                        else:
                            sql += f" WHERE {target_col} {op} ?"
                            params.append(val)

        # Сортировка как в C++
        sql += " ORDER BY length(key) ASC, key ASC"
        
        results = []
        try:
            with self._get_connection(table_name) as conn:
                cursor = conn.execute(sql, params)
                for row in cursor:
                    record = json.loads(row['value'])
                    record['__pk'] = row['key']
                    results.append(record)
        except Exception as e:
            print(f"NoSQL Error ({table_name}): {e}")
            
        return results

    def insert_record(self, table_name: str, pk_val: str, data: Dict[str, Any]):
        with self._get_connection(table_name) as conn:
            json_val = json.dumps(data, ensure_ascii=False)
            conn.execute("INSERT OR REPLACE INTO kv (key, value) VALUES (?, ?)", (str(pk_val), json_val))

    def update_record(self, table_name: str, pk_val: str, data: Dict[str, Any]):
        self.insert_record(table_name, pk_val, data)

    def delete_record(self, table_name: str, pk_val: str):
        with self._get_connection(table_name) as conn:
            conn.execute("DELETE FROM kv WHERE key = ?", (str(pk_val),))
