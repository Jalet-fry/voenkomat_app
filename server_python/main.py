import os
import json
from fastapi import FastAPI, HTTPException, Header, Query, Body
from fastapi.middleware.cors import CORSMiddleware
from typing import Optional, Dict, Any, List
from database import execute_query
from datetime import datetime

app = FastAPI(title="Voenkomat Backend API", version="1.3.1")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

SUPERUSER_PASSWORD = "admin"
# Справочники (LookUp Tables) - по ТЗ ЛР №1
LOOKUP_TABLES = ["fitness_categories", "commissioners"]

def is_superuser(token: str) -> bool:
    return token == SUPERUSER_PASSWORD

def get_pk_column(table_name: str) -> str:
    try:
        query = "SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = %s"
        res = execute_query(query, (table_name,))
        return res[0]['column_name'] if res else "id"
    except:
        return "id"

def escape_val(v: Any) -> str:
    if v is None or v == "" or v == "Не выбрано (NULL)" or v == "АВТО" or v == "--- Не выбрано (ПУСТО) ---":
        return "NULL"
    if isinstance(v, (int, float)):
        return str(v)
    escaped = str(v).replace("'", "''")
    return f"'{escaped}'"

def sync_bidirectional_links(table_name: str, record_id: Any, data: Dict[str, Any]):
    """Синхронизация связей (Лабораторная №1)"""
    try:
        # Исправлено: используем английские имена таблиц из новой схемы
        if table_name == "conscripts":
            p_id = record_id
            t_id = data.get("military_ticket_id")
            if t_id and t_id != "NULL":
                execute_query("UPDATE public.military_id_cards SET conscript_id = %s WHERE ticket_id = %s", (p_id, t_id))

            c_id = data.get("registration_card_id")
            if c_id and c_id != "NULL":
                execute_query("UPDATE public.service_record_cards SET conscript_id = %s WHERE card_id = %s", (p_id, c_id))
    except Exception as e:
        print(f"Sync warning: {e}")

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat API is running"}

@app.get("/api/all-tables")
def list_tables():
    res = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name;")
    return {"tables": [row['table_name'] for row in res]}

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    res = execute_query("SELECT column_name FROM information_schema.columns WHERE table_name = %s AND table_schema = 'public' ORDER BY ordinal_position", (table_name,))
    return {"columns": [r['column_name'] for r in res]}

@app.get("/api/column-details/{table_name}")
def get_col_details(table_name: str):
    res = execute_query("SELECT column_name as name, data_type as type, is_nullable = 'YES' as nullable, column_default as default FROM information_schema.columns WHERE table_name = %s AND table_schema = 'public' ORDER BY ordinal_position", (table_name,))
    return {"details": res}

@app.get("/api/foreign-keys/{table_name}")
def get_fks(table_name: str):
    query = """
    SELECT kcu.column_name as column, ccu.table_name AS referenced_table, ccu.column_name AS referenced_column
    FROM information_schema.table_constraints AS tc
    JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name
    JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name
    WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = %s
    """
    return {"foreign_keys": execute_query(query, (table_name,))}

@app.get("/api/metadata/{table_name}")
def get_meta(table_name: str):
    return {"pk": get_pk_column(table_name)}

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = Query(None)):
    query = f"SELECT * FROM public.{table_name}"
    if filters:
        query += f" WHERE {filters}"
    query += " ORDER BY 1 LIMIT 1000"
    return {"data": execute_query(query)}

@app.post("/api/execute-query")
async def run_custom_sql(payload: Dict[str, str] = Body(...), x_auth_token: Optional[str] = Header(None)):
    """Эндпоинт для окна SQL-запросов (QueriesWindow.cpp)"""
    # ВАЖНО: Разрешаем выполнение запросов, если передан корректный токен
    if not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Только администратор может выполнять произвольные SQL-запросы")

    sql = payload.get("sql", "")
    if not sql: raise HTTPException(status_code=400, detail="SQL пуст")
    try:
        res = execute_query(sql)
        # Если это SELECT, возвращаем данные, если INSERT/UPDATE/DELETE - инфо о затронутых строках
        return {"status": "success", "data": res}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Доступ запрещен для справочников")
    cleaned_data = {k: v for k, v in data.items() if v not in ["", "АВТО", None, "--- Не выбрано (ПУСТО) ---"]}
    cols = ", ".join(cleaned_data.keys())
    vals = list(cleaned_data.values())
    placeholders = ", ".join(["%s"] * len(vals))
    res = execute_query(f"INSERT INTO public.{table_name} ({cols}) VALUES ({placeholders}) RETURNING *", vals)
    return {"status": "success", "data": res[0]}

@app.put("/api/{table_name}/{record_id}")
async def update_rec(table_name: str, record_id: Any, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Доступ запрещен")
    pk = get_pk_column(table_name)
    sets = ", ".join([f"{k} = %s" for k in data.keys() if k != pk])
    vals = [v for k, v in data.items() if k != pk]
    vals.append(record_id)
    res = execute_query(f"UPDATE public.{table_name} SET {sets} WHERE {pk} = %s RETURNING *", vals)
    return {"status": "success", "data": res[0]}

@app.delete("/api/{table_name}/{record_id}")
async def del_rec(table_name: str, record_id: Any, x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Доступ запрещен")
    pk = get_pk_column(table_name)
    execute_query(f"DELETE FROM public.{table_name} WHERE {pk} = %s", (record_id,))
    return {"status": "success"}

@app.post("/api/backup")
async def create_backup(x_auth_token: Optional[str] = Header(None)):
    if not is_superuser(x_auth_token): raise HTTPException(status_code=403, detail="Доступ запрещен")
    tables_res = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'")
    backup_data = {t['table_name']: execute_query(f"SELECT * FROM {t['table_name']}") for t in tables_res}
    filename = f"backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    filepath = os.path.join(os.getcwd(), filename)
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(backup_data, f, ensure_ascii=False, indent=4, default=str)
    return {"status": "success", "file": filename}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
