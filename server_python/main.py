import os
import json
from fastapi import FastAPI, HTTPException, Body, Header, Query
from fastapi.responses import FileResponse
from fastapi.middleware.cors import CORSMiddleware
from typing import Optional, Dict, Any, List
from database import execute_query, get_db_connection
from datetime import datetime

app = FastAPI(title="Voenkomat Backend API", version="1.0.1")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

SUPERUSER_PASSWORD = "admin"
LOOKUP_TABLES = ["fitness_categories", "commissioners"]
MAIN_TABLE = "callup_events" 

def is_superuser(token: str) -> bool:
    return token == SUPERUSER_PASSWORD

def get_pk_column(table_name: str) -> str:
    query = f"SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'"
    try:
        res = execute_query(query)
        return res[0]['column_name'] if res else "id"
    except:
        return "id"

def escape_val(v: Any) -> str:
    if v is None or v == "": return "NULL"
    if isinstance(v, (int, float)): return str(v)
    escaped = str(v).replace("'", "''")
    return f"'{escaped}'"

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat API is running", "main_table": MAIN_TABLE}

# --- СИСТЕМНЫЕ МАРШРУТЫ (НЕОБХОДИМЫ ДЛЯ КЛИЕНТА) ---

@app.get("/api/all-tables")
def list_tables():
    res = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name;")
    return {"tables": [row['table_name'] for row in res]}

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    res = execute_query(f"SELECT column_name FROM information_schema.columns WHERE table_name = '{table_name}' AND table_schema = 'public' ORDER BY ordinal_position")
    return {"columns": [r['column_name'] for r in res]}

@app.get("/api/metadata/{table_name}")
def get_meta(table_name: str):
    pk = get_pk_column(table_name)
    return {"pk": pk}

@app.get("/api/column-details/{table_name}")
def get_details(table_name: str):
    query = f"SELECT column_name as name, data_type as type, is_nullable = 'YES' as nullable, column_default as default FROM information_schema.columns WHERE table_name = '{table_name}' AND table_schema = 'public' ORDER BY ordinal_position"
    return {"details": execute_query(query)}

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...)):
    sql = payload.get("sql")
    if not sql: raise HTTPException(status_code=400, detail="SQL query missing")
    return {"data": execute_query(sql)}

# --- ТРЕБОВАНИЯ ЛАБ 1 (БЭКАП) ---

@app.post("/api/backup")
async def create_backup(x_auth_token: Optional[str] = Header(None)):
    if not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Доступ запрещен")
    
    tables = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'")
    backup_data = {t['table_name']: execute_query(f"SELECT * FROM {t['table_name']}") for t in tables}
    
    filename = f"backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    filepath = os.path.join(os.getcwd(), filename)
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(backup_data, f, ensure_ascii=False, indent=4, default=str)
    
    return {"status": "success", "file": filename}

# --- CRUD С ПРОВЕРКОЙ ПРАВ ---

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = Query(None)):
    query = f"SELECT * FROM public.{table_name}"
    if filters: query += f" WHERE {filters}"
    query += " ORDER BY 1 LIMIT 1000"
    return {"data": execute_query(query)}

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Запрещено изменять справочники")
    
    cols = ", ".join(data.keys())
    vals = ", ".join([escape_val(v) for v in data.values()])
    query = f"INSERT INTO public.{table_name} ({cols}) VALUES ({vals}) RETURNING *"
    res = execute_query(query)
    return {"status": "success", "data": res[0]}

@app.put("/api/{table_name}/{record_id}")
async def update_rec(table_name: str, record_id: Any, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Запрещено изменять справочники")
    
    pk = get_pk_column(table_name)
    sets = ", ".join([f"{k} = {escape_val(v)}" for k, v in data.items() if k != pk])
    query = f"UPDATE public.{table_name} SET {sets} WHERE {pk} = {escape_val(record_id)} RETURNING *"
    res = execute_query(query)
    return {"status": "success", "data": res[0]}

@app.delete("/api/{table_name}/{record_id}")
async def del_rec(table_name: str, record_id: Any, x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Запрещено удалять из справочников")

    pk = get_pk_column(table_name)
    execute_query(f"DELETE FROM public.{table_name} WHERE {pk} = {escape_val(record_id)}")
    return {"status": "success"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)
