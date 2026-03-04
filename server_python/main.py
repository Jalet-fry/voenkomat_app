import os
from fastapi import FastAPI, HTTPException, Body, Header
from fastapi.middleware.cors import CORSMiddleware
from typing import Optional, Dict, Any
from database import execute_query, get_db_connection

app = FastAPI(title="Voenkomat Backend API", version="2.4.1")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

SUPERUSER_PASSWORD = "admin"

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
    # Исправлено: избегаем обратных слешей внутри f-строки для совместимости
    escaped = str(v).replace("'", "''")
    return f"'{escaped}'"

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat API is running"}

# --- СТАТИЧЕСКИЕ МАРШРУТЫ (ОБЯЗАТЕЛЬНО ВЫШЕ ДИНАМИЧЕСКИХ {table_name}) ---

@app.get("/api/all-tables")
def list_tables():
    res = execute_query("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name;")
    return {"tables": [row['table_name'] for row in res]}

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...)):
    sql = payload.get("sql")
    if not sql:
        raise HTTPException(status_code=400, detail="SQL query text is missing.")
    try:
        result = execute_query(sql)
        return {"data": result}
    except Exception as e:
        print(f"SQL Execution Error: {e}")
        raise HTTPException(status_code=400, detail=str(e))

@app.post("/api/restore")
async def restore_db(payload: Dict[str, str] = Body(...), x_auth_token: Optional[str] = Header(None)):
    if x_auth_token != SUPERUSER_PASSWORD:
        raise HTTPException(status_code=403, detail="Доступ запрещен")
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(payload.get("sql", ""))
        conn.commit()
        return {"status": "success"}
    except Exception as e:
        conn.rollback()
        raise HTTPException(status_code=500, detail=str(e))
    finally:
        conn.close()

# --- ДИНАМИЧЕСКИЕ МАРШРУТЫ ---

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    res = execute_query(f"SELECT column_name FROM information_schema.columns WHERE table_name = '{table_name}' ORDER BY ordinal_position")
    return {"columns": [r['column_name'] for r in res]}

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = None):
    query = f"SELECT * FROM public.{table_name}"
    if filters:
        query += f" WHERE {filters}"
    query += " ORDER BY 1 LIMIT 1000"
    return {"data": execute_query(query)}

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any]):
    cols = ", ".join(data.keys())
    vals = ", ".join([escape_val(v) for v in data.values()])
    query = f"INSERT INTO public.{table_name} ({cols}) VALUES ({vals}) RETURNING *"
    try:
        res = execute_query(query)
        return {"status": "success", "data": res[0]}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.put("/api/{table_name}/{record_id}")
async def update_rec(table_name: str, record_id: Any, data: Dict[str, Any]):
    pk = get_pk_column(table_name)
    sets = ", ".join([f"{k} = {escape_val(v)}" for k, v in data.items() if k != pk])
    query = f"UPDATE public.{table_name} SET {sets} WHERE {pk} = {escape_val(record_id)} RETURNING *"
    try:
        res = execute_query(query)
        return {"status": "success", "data": res[0]}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.delete("/api/{table_name}/{record_id}")
async def del_rec(table_name: str, record_id: Any):
    pk = get_pk_column(table_name)
    execute_query(f"DELETE FROM public.{table_name} WHERE {pk} = {escape_val(record_id)}")
    return {"status": "success"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)
