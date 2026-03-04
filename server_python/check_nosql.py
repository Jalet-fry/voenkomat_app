import dbm
import json
import os

def inspect_nosql():
    nosql_dir = "nosql_db"
    
    if not os.path.exists(nosql_dir):
        print(f"Ошибка: Директория {nosql_dir} не найдена. Сначала запустите converter.py")
        return

    # Получаем список всех баз (dbm на Windows создает .dat/.dir, берем имя без расширения)
    db_files = set(f.replace(".db.dat", "").replace(".db.dir", "").replace(".db.bak", "") 
                   for f in os.listdir(nosql_dir) if f.endswith(".db.dat") or f.endswith(".db.dir"))

    if not db_files:
        print("Баз данных NoSQL не обнаружено.")
        return

    print("=== ПРОВЕРКА СОДЕРЖИМОГО NoSQL (BerkeleyDB/dbm) ===\n")

    for db_name in sorted(db_files):
        db_path = os.path.join(nosql_dir, db_name + ".db")
        print(f"--- Таблица: {db_name} ---")
        
        try:
            with dbm.open(db_path, 'r') as db:
                # Получаем все ключи
                keys = db.keys()
                print(f"Всего записей: {len(keys)}")
                
                # Показываем первые 3 записи для проверки формата
                sample_count = min(3, len(keys))
                for i in range(sample_count):
                    key = keys[i]
                    value = db[key]
                    
                    # Декодируем байты в строки
                    k_str = key.decode('utf-8')
                    v_str = value.decode('utf-8')
                    
                    # Пытаемся распарсить JSON для красивого вывода
                    try:
                        v_json = json.loads(v_str)
                        print(f"  КЛЮЧ (PK): {k_str}")
                        print(f"  ЗНАЧЕНИЕ (JSON): {json.dumps(v_json, indent=4, ensure_ascii=False)}")
                    except:
                        print(f"  КЛЮЧ: {k_str}")
                        print(f"  ЗНАЧЕНИЕ: {v_str}")
                    print("-" * 20)
        except Exception as e:
            print(f"  Ошибка чтения {db_name}: {e}")
        print("\n")

if __name__ == "__main__":
    inspect_nosql()
