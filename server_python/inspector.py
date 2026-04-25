import dbm
import os
import json
import sys

def inspect(table_name=None):
    nosql_dir = os.path.join(os.path.dirname(__file__), "..", "nosql_db_python")
    
    # Если имя не передано, берем все .db из папки (ориентируемся по .dir или .dat)
    if not table_name:
        files = [f.replace('.dir', '').replace('.dat', '').replace('.db', '') 
                 for f in os.listdir(nosql_dir) if f.endswith('.dir')]
        tables = sorted(list(set(files)))
    else:
        tables = [table_name.replace('.db', '')]

    print(f"{'='*60}")
    print(f"{'КЛЮЧ':<15} | {'ДАННЫЕ (JSON)'}")
    print(f"{'='*60}")

    for table in tables:
        db_path = os.path.join(nosql_dir, table + ".db")
        print(f"\n>>> ТАБЛИЦА: {table.upper()}")
        
        try:
            # Открываем в режиме чтения 'r'
            with dbm.open(db_path, 'r') as db:
                for key in db.keys():
                    k_str = key.decode('utf-8')
                    v_str = db[key].decode('utf-8')
                    print(f"{k_str:<15} | {v_str}")
        except Exception as e:
            print(f"Ошибка чтения {table}: {e}")

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else None
    inspect(target)
