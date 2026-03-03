import os
from fastapi import FastAPI, HTTPException, Query, Body, Header
from fastapi.middleware.cors import CORSMiddleware
from typing import Optional, Dict, Any, List
import json

from database import execute_query, get_db_connection

app = FastAPI(title="Voenkomat Backend API", version="2.3.1")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

SUPERUSER_PASSWORD = "admin"

TABLE_MAPPING = {
    "conscripts": "conscripts",
    "commissioners": "commissioners",
    "medical_examinations": "medical_examinations",
    "fitness_categories": "fitness_categories",
    "military_id_cards": "military_id_cards",
    "service_record_cards": "service_record_cards",
    "callup_events": "callup_events"
}

LOOKUP_TABLES = ["fitness_categories", "positions", "lesson_types", "groups", "subjects"]

def get_pk_column(table_name: str) -> str:
    query = f"SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'"
    res = execute_query(query)
    return res[0]['column_name'] if res else "id"

def get_real_table_name(table_name: str) -> str:
    clean_name = table_name.replace("-", "_")
    name = TABLE_MAPPING.get(clean_name, clean_name)
    if not name.replace("_", "").isalnum():
        raise HTTPException(status_code=400, detail="Invalid table name")
    return name

def escape_val(v: Any) -> str:
    if v is None or v == "": return "NULL"
    if isinstance(v, (int, float)): return str(v)
    s = str(v).replace("'", "''")
    return f"'{s}'"

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat API"}

@app.get("/api/all-tables")
def list_tables():
    query = "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name;"
    res = execute_query(query)
    return {"tables": [row['table_name'] for row in res]}

@app.post("/api/restore")
async def restore_database(payload: Dict[str, str] = Body(...), x_auth_token: Optional[str] = Header(None)):
    if x_auth_token != SUPERUSER_PASSWORD: raise HTTPException(status_code=403, detail="Forbidden")
    sql = payload.get("sql", "")
    commands = [c.strip() for c in sql.split(';') if c.strip()]
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            for cmd in commands: cur.execute(cmd)
        conn.commit()
        return {"status": "success"}
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally: conn.close()

@app.post("/api/create-table")
async def create_table(payload: Dict[str, Any] = Body(...), x_auth_token: Optional[str] = Header(None)):
    if x_auth_token != SUPERUSER_PASSWORD: raise HTTPException(status_code=403, detail="Forbidden")
    name = payload.get("table_name")
    cols = payload.get("columns", [])
    pks = payload.get("primary_keys", [])
    defs = [f"{c['name']} {c['type']}" for c in cols]
    if pks: defs.append(f"PRIMARY KEY ({', '.join(pks)})")
    try:
        execute_query(f"CREATE TABLE public.{name} ({', '.join(defs)})")
        return {"status": "success"}
    except Exception as e: raise HTTPException(status_code=400, detail=str(e))

@app.get("/api/metadata/{table_name}")
def get_metadata(table_name: str):
    real = get_real_table_name(table_name)
    return {"pk": get_pk_column(real), "is_lookup": real in LOOKUP_TABLES}

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...)):
    try:
        result = execute_query(payload.get("sql"))
        return {"data": result}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = None):
    real = get_real_table_name(table_name)
    query = f"SELECT * FROM public.{real}"
    if filters: query += f" WHERE {filters}"
    query += " ORDER BY 1 LIMIT 1000"
    return {"data": execute_query(query)}

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any]):
    real = get_real_table_name(table_name)
    cols = ", ".join(data.keys())
    vals = ", ".join([escape_val(v) for v in data.values()])
    query = f"INSERT INTO public.{real} ({cols}) VALUES ({vals}) RETURNING *"
    try:
        res = execute_query(query)
        return {"status": "success", "data": res[0]}
    except Exception as e: raise HTTPException(status_code=400, detail=str(e))

@app.put("/api/{table_name}/{id}")
async def update_rec(table_name: str, id: Any, data: Dict[str, Any]):
    real = get_real_table_name(table_name)
    pk = get_pk_column(real)
    sets = ", ".join([f"{k} = {escape_val(v)}" for k, v in data.items() if k != pk])
    try:
        res = execute_query(f"UPDATE public.{real} SET {sets} WHERE {pk} = {id} RETURNING *")
        return {"status": "success", "data": res[0]}
    except Exception as e: raise HTTPException(status_code=400, detail=str(e))

@app.delete("/api/{table_name}/{id}")
async def del_rec(table_name: str, id: Any):
    real = get_real_table_name(table_name)
    pk = get_pk_column(real)
    execute_query(f"DELETE FROM public.{real} WHERE {pk} = {id}")
    return {"status": "success"}

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    real = get_real_table_name(table_name)
    res = execute_query(f"SELECT column_name FROM information_schema.columns WHERE table_name = '{real}' ORDER BY ordinal_position")
    return {"columns": [r['column_name'] for r in res]}

@app.get("/api/column-details/{table_name}")
def get_details(table_name: str):
    real = get_real_table_name(table_name)
    return {"details": execute_query(f"SELECT column_name as name, data_type as type, is_nullable = 'YES' as nullable, character_maximum_length as length FROM information_schema.columns WHERE table_name = '{real}'")}

if __name__ == "__main__":
    import uvicorn
    # Запускаем строго на localhost (127.0.0.1)
    uvicorn.run(app, host="127.0.0.1", port=8000)
