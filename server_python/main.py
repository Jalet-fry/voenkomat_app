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

# Справочники (Только для чтения для обычных пользователей)
LOOKUP_TABLES = ["fitness_categories", "commissioners", "positions"]

def check_superuser(token: str):
    if token != SUPERUSER_PASSWORD:
        raise HTTPException(status_code=403, detail="Доступ запрещен: требуются права администратора")

def get_pk_column(table_name: str) -> str:
    query = f"SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = '{table_name}'"
    res = execute_query(query)
    return res[0]['column_name'] if res else "id"

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
    check_superuser(x_auth_token)
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

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = None):
    query = f"SELECT * FROM public.{table_name}"
    if filters: query += f" WHERE {filters}"
    query += " ORDER BY 1 LIMIT 1000"
    return {"data": execute_query(query)}

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES:
        check_superuser(x_auth_token)
    
    cols = ", ".join(data.keys())
    vals = ", ".join([escape_val(v) for v in data.values()])
    query = f"INSERT INTO public.{table_name} ({cols}) VALUES ({vals}) RETURNING *"
    try:
        res = execute_query(query)
        return {"status": "success", "data": res[0]}
    except Exception as e: raise HTTPException(status_code=400, detail=str(e))

@app.put("/api/{table_name}/{id}")
async def update_rec(table_name: str, id: Any, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES:
        check_superuser(x_auth_token)
        
    pk = get_pk_column(table_name)
    sets = ", ".join([f"{k} = {escape_val(v)}" for k, v in data.items() if k != pk])
    try:
        res = execute_query(f"UPDATE public.{table_name} SET {sets} WHERE {pk} = {id} RETURNING *")
        return {"status": "success", "data": res[0]}
    except Exception as e: raise HTTPException(status_code=400, detail=str(e))

@app.delete("/api/{table_name}/{id}")
async def del_rec(table_name: str, id: Any, x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES:
        check_superuser(x_auth_token)
        
    pk = get_pk_column(table_name)
    execute_query(f"DELETE FROM public.{table_name} WHERE {pk} = {id}")
    return {"status": "success"}

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    res = execute_query(f"SELECT column_name FROM information_schema.columns WHERE table_name = '{table_name}' ORDER BY ordinal_position")
    return {"columns": [r['column_name'] for r in res]}

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...)):
    try:
        result = execute_query(payload.get("sql"))
        return {"data": result}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)
