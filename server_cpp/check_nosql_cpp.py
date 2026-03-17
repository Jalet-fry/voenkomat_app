import json
import os

def check_cpp_nosql():
    # Путь к папке, которую создает C++ конвертер
    nosql_dir = os.path.join(os.path.dirname(__file__), "build", "Desktop_Qt_6_10_2_MinGW_64_bit-Release", "nosql_db_cpp")
    
    if not os.path.exists(nosql_dir):
        print(f"ОШИБКА: Папка {nosql_dir} не найдена!")
        print("Сначала запустите run_nosql.bat")
        return

    print("=== АУДИТ NoSQL БАЗЫ (C++ VERSION) ===\n")
    
    files = [f for f in os.listdir(nosql_dir) if f.endswith(".db.txt")]
    
    for filename in sorted(files):
        table_name = filename.replace(".db.txt", "")
        print(f"ТАБЛИЦА: {table_name}")
        
        with open(os.path.join(nosql_dir, filename), 'r', encoding='utf-8') as f:
            lines = f.readlines()
            print(f"  Записей: {len(lines)}")
            
            # Проверяем первую строку для аудита формата
            if lines:
                parts = lines[0].strip().split(" ||| ")
                if len(parts) == 2:
                    key, val = parts[0], parts[1]
                    print(f"  ПРИМЕР КЛЮЧА: {key}")
                    
                    # Проверяем JSON
                    try:
                        js = json.loads(val)
                        print(f"  ПРИМЕР JSON (структура): {list(js.keys())}")
                        # Проверка соответствия ТЗ (исключение PK из JSON)
                        if "_" not in key and key.isdigit():
                             if any(k in js for k in ["id", "conscript_id", "event_id", "id_bileta"]):
                                 print("  [!] Предупреждение: PK найден внутри JSON (не критично, но проверьте ТЗ)")
                    except:
                        print("  [!] ОШИБКА: Невалидный JSON!")
                else:
                    print("  [!] ОШИБКА: Неверный разделитель (нужен ' ||| ')")
        print("-" * 30)

if __name__ == "__main__":
    check_cpp_nosql()
