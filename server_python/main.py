import os
import subprocess
import datetime
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
import pandas as pd
from typing import Optional, Dict, Any

# Импортируем функции из нашего модуля базы данных
from database import execute_query, get_db_connection, get_db_config

app = FastAPI(title="Voenkomat Backend API", version="1.5.0")

# Настройка CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Словарь для соответствия URL и имен таблиц
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
    return {"message": "Voenkomat Database API", "version": "1.5.0"}

@app.get("/api/{table_name}")
async def get_table(
    table_name: str,
    id: Optional[int] = None,
    page: int = Query(1, ge=1),
    limit: int = Query(100, ge=1, le=1000)
):
    """Получить данные из таблицы"""
    db_table_name = TABLE_MAPPING.get(table_name)
    if not db_table_name:
        # Пытаемся использовать имя напрямую, если его нет в маппинге
        db_table_name = table_name

    query = f"SELECT * FROM public.{db_table_name}"
    params = []

    if id:
        # Предполагаем, что имя PK совпадает с pattern {table_singular}_id
        # Но для простоты в API будем использовать переданный ID
        query += f" WHERE {db_table_name[:-1]}_id = %s"
        params.append(id)

    offset = (page - 1) * limit
    query += f" LIMIT {limit} OFFSET {offset}"

    try:
        result = execute_query(query, params)
        return {"data": result, "page": page, "limit": limit}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/api/export/xlsx/{table_name}")
async def export_xlsx(table_name: str):
    """Экспорт таблицы в Excel"""
    db_table_name = TABLE_MAPPING.get(table_name, table_name)
    try:
        conn = get_db_connection()
        df = pd.read_sql(f"SELECT * FROM public.{db_table_name}", conn)
        conn.close()
        
        export_dir = os.path.join(os.path.dirname(__file__), "..", "exports", "tables")
        os.makedirs(export_dir, exist_ok=True)
        file_path = os.path.join(export_dir, f"{db_table_name}.xlsx")
        df.to_excel(file_path, index=False)
        
        return FileResponse(file_path, filename=f"{db_table_name}.xlsx")
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/special/report-6.19")
async def special_report():
    """Специальный отчет из Лабораторной работы №6"""
    query = """
        SELECT mo.examination_date, p.full_name, kg.category_name 
        FROM public.medical_examinations mo 
        JOIN public.conscripts p ON p.conscript_id = mo.conscript_id 
        JOIN public.fitness_categories kg ON kg.category_id = mo.category_id 
        ORDER BY mo.examination_date DESC;
    """
    try:
        result = execute_query(query)
        return {"data": result}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/api/backup")
async def create_backup():
    """Создать резервную копию базы данных"""
    db_params = get_db_config()
    timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    backup_dir = os.path.join(os.path.dirname(__file__), "..", "exports", "backups")
    os.makedirs(backup_dir, exist_ok=True)
    filepath = os.path.join(backup_dir, f"backup_{timestamp}.sql")
    
    os.environ["PGPASSWORD"] = db_params["password"]
    cmd = [
        "pg_dump",
        "-h", db_params["host"],
        "-U", db_params["user"],
        "-f", filepath,
        db_params["database"]
    ]
    
    try:
        subprocess.run(cmd, check=True)
        return {"status": "success", "file": filepath}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# Это входная точка приложения (Method Main в Python)
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)
