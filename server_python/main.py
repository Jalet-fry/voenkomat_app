import os
import json
from fastapi import FastAPI, HTTPException, Header, Query, Body
from fastapi.middleware.cors import CORSMiddleware
from typing import Optional, Dict, Any, List
from datetime import datetime
from nosql_manager import NoSQLManager

app = FastAPI(title="Voenkomat NoSQL API", version="1.4.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

SUPERUSER_PASSWORD = "admin"
LOOKUP_TABLES = ["fitness_categories", "commissioners"]
db_manager = NoSQLManager()

# Таблица метаданных для NoSQL (соответствие PK именам колонок)
# Т.к. NoSQL хранит данные как ID ||| JSON, нам нужно знать, как назывался ID в SQL
TABLE_PK_MAP = {
    "conscripts": "conscript_id",
    "commissioners": "commissioner_id",
    "fitness_categories": "category_id",
    "medical_examinations": "certification_id",
    "military_id_cards": "ticket_id",
    "service_record_cards": "card_id",
    "callup_events": "event_id",
    "conscripts_events": "id",
    "conscripts_commissioners": "id",
    "test_table": "id"
}

def is_superuser(token: str) -> bool:
    return token == SUPERUSER_PASSWORD

@app.get("/")
async def root():
    return {"status": "ok", "message": "Voenkomat NoSQL API is running"}

@app.get("/api/all-tables")
def list_tables():
    return {"tables": db_manager.list_tables()}

@app.get("/api/columns/{table_name}")
def get_cols(table_name: str):
    data = db_manager.get_all_data(table_name)
    if not data:
        return {"columns": []}
    
    # Реконструируем список колонок
    pk_col = TABLE_PK_MAP.get(table_name, "id")
    cols = [pk_col]
    
    # Берем остальные ключи из JSON первой записи, исключая PK
    other_cols = [k for k in data[0].keys() if k != "__pk" and k != pk_col]
    cols.extend(other_cols)
    
    return {"columns": cols}

@app.get("/api/column-details/{table_name}")
def get_col_details(table_name: str):
    # Упрощенная реализация для NoSQL
    cols = get_cols(table_name)["columns"]
    details = []
    for c in cols:
        details.append({
            "name": c,
            "type": "text", # В NoSQL типы размыты
            "nullable": True,
            "default": None
        })
    return {"details": details}

@app.get("/api/foreign-keys/{table_name}")
def get_fks(table_name: str):
    # Emulate Foreign Keys for NoSQL to allow link-selection in RecordDialog
    # Qt client expects: "column", "referenced_table", "referenced_column"
    fks = []
    if table_name == "medical_examinations":
        fks.append({"column": "conscript_id", "referenced_table": "conscripts", "referenced_column": "conscript_id"})
        fks.append({"column": "category_id", "referenced_table": "fitness_categories", "referenced_column": "category_id"})
    elif table_name == "military_id_cards":
        fks.append({"column": "conscript_id", "referenced_table": "conscripts", "referenced_column": "conscript_id"})
    elif table_name == "service_record_cards":
        fks.append({"column": "conscript_id", "referenced_table": "conscripts", "referenced_column": "conscript_id"})
    elif table_name == "callup_events":
        fks.append({"column": "conscript_id", "referenced_table": "conscripts", "referenced_column": "conscript_id"})
        fks.append({"column": "commissioner_id", "referenced_table": "commissioners", "referenced_column": "commissioner_id"})
    
    return {"foreign_keys": fks}

@app.get("/api/metadata/{table_name}")
def get_meta(table_name: str):
    return {"pk": TABLE_PK_MAP.get(table_name, "id")}

@app.get("/api/{table_name}")
async def get_data(table_name: str, filters: Optional[str] = Query(None)):
    pk_col = TABLE_PK_MAP.get(table_name, "id")
    
    # If filter is on PK column, we should map it to __pk for the manager
    manager_filters = filters
    if filters and pk_col in filters:
        manager_filters = filters.replace(pk_col, "__pk")

    raw_data = db_manager.execute_filter(table_name, manager_filters)
    
    formatted_data = []
    for row in raw_data:
        # row содержит {'__pk': key, 'col1': val1, ...}
        # Нам нужно отдать {'id_name': key, 'col1': val1, ...}
        # Исключаем дублирование: если в JSON уже есть pk_col, берем его значение или заменяем на __pk
        record = {pk_col: row.get('__pk')}
        for k, v in row.items():
            if k not in ["__pk", pk_col]:
                record[k] = v
        formatted_data.append(record)
    
    # Сортировка по числовому ID для красоты
    try:
        formatted_data.sort(key=lambda x: int(x[pk_col]) if str(x[pk_col]).isdigit() else str(x[pk_col]))
    except:
        pass
        
    return {"data": formatted_data}

@app.post("/api/execute-query")
async def run_custom_query(payload: Dict[str, str] = Body(...), x_auth_token: Optional[str] = Header(None)):
    if not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Forbidden")
    
    sql = payload.get("sql", "").lower()
    # Эмуляция SQL для NoSQL в рамках ЛР4
    if "select" in sql and "from conscripts" in sql:
        return {"status": "success", "data": (await get_data("conscripts"))["data"]}
        
    raise HTTPException(status_code=400, detail="SQL queries are not supported in NoSQL mode. Use direct API.")

@app.post("/api/{table_name}")
async def add_rec(table_name: str, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Forbidden")
    
    pk_col = TABLE_PK_MAP.get(table_name, "id")
    # Генерируем новый ID если нет
    new_id = data.get(pk_col) or str(int(datetime.now().timestamp()))
    
    payload = {k: v for k, v in data.items() if k != pk_col}
    db_manager.insert_record(table_name, new_id, payload)
    
    return {"status": "success", "data": {pk_col: new_id, **payload}}

@app.put("/api/{table_name}/{record_id}")
async def update_rec(table_name: str, record_id: Any, data: Dict[str, Any], x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Forbidden")
        
    pk_col = TABLE_PK_MAP.get(table_name, "id")
    payload = {k: v for k, v in data.items() if k != pk_col}
    db_manager.update_record(table_name, record_id, payload)
    
    return {"status": "success", "data": {pk_col: record_id, **payload}}

@app.delete("/api/{table_name}/{record_id}")
async def del_rec(table_name: str, record_id: Any, x_auth_token: Optional[str] = Header(None)):
    if table_name in LOOKUP_TABLES and not is_superuser(x_auth_token):
        raise HTTPException(status_code=403, detail="Forbidden")
    db_manager.delete_record(table_name, record_id)
    return {"status": "success"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
