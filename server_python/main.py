import os
import subprocess
import datetime
from fastapi import FastAPI, HTTPException, Query, Body
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
import pandas as pd
from typing import Optional, Dict, Any, List

from database import execute_query, get_db_connection, get_db_config

app = FastAPI(title="Voenkomat Backend API", version="1.7.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

TABLE_MAPPING = {
    "conscripts": "conscripts",
    "commissioners": "commissioners",
    "medical-examinations": "medical_examinations",
    "fitness-categories": "fitness_categories",
    "military-id-cards": "military_id_cards",
    "service-record-cards": "service_record_cards",
    "callup-events": "callup_events"
}

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat Database API is running"}

@app.get("/api/all-tables")
def list_tables():
    query = "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name;"
    try:
        result = execute_query(query)
        return {"tables": [row['table_name'] for row in result]}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/api/{table_name}")
async def get_table(table_name: str, page: int = 1, limit: int = 1000):
    db_table_name = TABLE_MAPPING.get(table_name, table_name)
    # Простейшая защита от инъекций в имени таблицы
    if not db_table_name.replace("_", "").isalnum():
        raise HTTPException(status_code=400, detail="Invalid table name")

    query = f"SELECT * FROM public.{db_table_name} LIMIT {limit} OFFSET {(page-1)*limit}"
    try:
        result = execute_query(query)
        return {"data": result}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...)):
    sql = payload.get("sql")
    if not sql:
        raise HTTPException(status_code=400, detail="No SQL provided")
    try:
        result = execute_query(sql)
        # Если это SELECT, возвращаем данные, иначе количество затронутых строк
        return {"data": result}
    except Exception as e:
        print(f"SQL Error: {e}")
        raise HTTPException(status_code=400, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000) # Слушаем на всех интерфейсах
