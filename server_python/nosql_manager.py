import sqlite3
import json
import os
import re
from typing import List, Dict, Any, Optional

NOSQL_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "nosql_db_python"))

class NoSQLManager:
    def __init__(self):
        if not os.path.exists(NOSQL_DIR):
            os.makedirs(NOSQL_DIR)

    def _get_db_path(self, table_name: str) -> str:
        return os.path.join(NOSQL_DIR, f"{table_name}.db")

    def _get_connection(self, table_name: str):
        db_path = self._get_db_path(table_name)
        conn = sqlite3.connect(db_path)
        # Создаем структуру "Ключ-Значение" (как в Berkeley DB SQL)
        conn.execute("CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT)")
        return conn

    def list_tables(self) -> List[str]:
        if not os.path.exists(NOSQL_DIR): return []
        # Теперь просто ищем файлы .db
        return [f.replace('.db', '') for f in os.listdir(NOSQL_DIR) if f.endswith('.db')]

    def get_all_data(self, table_name: str) -> List[Dict[str, Any]]:
        if not os.path.exists(self._get_db_path(table_name)): return []
        
        results = []
        try:
            with self._get_connection(table_name) as conn:
                try:
                    cursor = conn.execute(f"SELECT key, value FROM kv")
                except:
                    cursor = conn.execute(f"SELECT key, value FROM {table_name}")
                
                for key, value in cursor:
                    row = json.loads(value)
                    # Сохраняем ключ отдельно для фильтрации
                    row['__pk'] = key
                    results.append(row)
        except Exception as e:
            print(f"Error reading {table_name}: {e}")
        return results

    def insert_record(self, table_name: str, pk_val: str, data: Dict[str, Any]):
        with self._get_connection(table_name) as conn:
            json_val = json.dumps(data, ensure_ascii=False)
            conn.execute("INSERT OR REPLACE INTO kv (key, value) VALUES (?, ?)", (str(pk_val), json_val))

    def update_record(self, table_name: str, pk_val: str, data: Dict[str, Any]):
        self.insert_record(table_name, pk_val, data) # В KV это одно и то же

    def delete_record(self, table_name: str, pk_val: str):
        with self._get_connection(table_name) as conn:
            conn.execute("DELETE FROM kv WHERE key = ?", (str(pk_val),))

    def execute_filter(self, table_name: str, filter_str: Optional[str] = None) -> List[Dict[str, Any]]:
        data = self.get_all_data(table_name)
        if not filter_str: return data
        
        try:
            # 1. Попытка распарсить ILIKE (поиск подстроки)
            # Поддерживает: "col::text ILIKE '%val%'" и "col ILIKE val"
            ilike_pattern = r"(\w+)(?:::text)?\s+ILIKE\s+['%]*([^'%]+)['%]*"
            ilike_match = re.search(ilike_pattern, filter_str, re.IGNORECASE)
            if ilike_match:
                col, val = ilike_match.groups()
                val = val.lower()
                return [row for row in data if val in str(row.get(col, "")).lower() or (col == "__pk" and val in str(row["__pk"]).lower())]

            # 2. Попытка распарсить операторы сравнения (=, >, <, !=)
            # Поддерживает: "col = val", "col > 5" и т.д.
            match = re.search(r"(\w+)\s*([=><!]{1,2})\s*(.+)", filter_str)
            if match:
                col, op, val = match.groups()
                val = val.strip("'\" ")
                
                results = []
                for row in data:
                    actual_val = row.get(col)
                    if actual_val is None and col == "__pk":
                        actual_val = row.get("__pk")
                    
                    if actual_val is None: continue

                    try:
                        # Пытаемся сравнивать как числа
                        target_v, actual_v = float(val), float(actual_val)
                    except:
                        # Иначе как строки
                        target_v, actual_v = str(val).lower(), str(actual_val).lower()

                    if (op == "=" or op == "==") and actual_v == target_v: results.append(row)
                    elif (op == "!=" or op == "<>") and actual_v != target_v: results.append(row)
                    elif op == ">" and actual_v > target_v: results.append(row)
                    elif op == "<" and actual_v < target_v: results.append(row)
                    elif op == ">=" and actual_v >= target_v: results.append(row)
                    elif op == "<=" and actual_v <= target_v: results.append(row)
                return results
            
            return data # Если не распарсили, возвращаем всё
        except Exception as e:
            print(f"Filter error: {e}")
            return data
