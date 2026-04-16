import json
import os

def check_cpp_nosql():
    # Ищем папку в корне проекта или в папке сервера
    base_dir = os.path.dirname(os.path.dirname(__file__))
    nosql_dir = os.path.join(base_dir, "nosql_db_cpp")
    
    if not os.path.exists(nosql_dir):
        # Проверка внутри папки сервера (на случай запуска из разных мест)
        nosql_dir = os.path.join(os.path.dirname(__file__), "nosql_db_cpp")

    if not os.path.exists(nosql_dir):
        print(f"ОШИБКА: Папка nosql_db_cpp не найдена!")
        print(f"Ожидалось тут: {nosql_dir}")
        return

    print("=== АУДИТ NoSQL БАЗЫ (C++ VERSION) ===\n")
    
    # Сначала проверяем бинарные файлы (наличие)
    bin_files = [f for f in os.listdir(nosql_dir) if f.endswith(".db") and not f.endswith(".txt")]
    print(f"Найдено БИНАРНЫХ баз (.db): {len(bin_files)}")
    for bf in bin_files:
        size = os.path.getsize(os.path.join(nosql_dir, bf))
        print(f"  - {bf} ({size} байт) [STATUS: OK]")
    
    print("\n" + "="*40 + "\n")

    # Читаем текстовые дампы для проверки содержимого
    txt_files = [f for f in os.listdir(nosql_dir) if f.endswith(".db.txt")]
    
    for filename in sorted(txt_files):
        table_name = filename.replace(".db.txt", "")
        print(f"АНАЛИЗ ТАБЛИЦЫ: {table_name}")
        
        try:
            with open(os.path.join(nosql_dir, filename), 'r', encoding='utf-8') as f:
                lines = f.readlines()
                print(f"  Записей: {len(lines)}")
                
                if lines:
                    parts = lines[0].strip().split(" ||| ")
                    if len(parts) == 2:
                        key, val = parts[0], parts[1]
                        print(f"  Ключ (PK): {key}")
                        
                        js = json.loads(val)
                        print(f"  Данные (JSON): {json.dumps(js, ensure_ascii=False)[:80]}...")
                        
                        # Проверка на исключение PK из JSON (требование ЛР3)
                        if key.isdigit() and any(k in js.lower() for k in ["id", "id_"+table_name]):
                            print("  [!] Замечание: PK дублируется в JSON (не критично)")
                    else:
                        print("  [!] Ошибка формата строки")
        except Exception as e:
            print(f"  [!] Ошибка чтения: {e}")
        print("-" * 30)

if __name__ == "__main__":
    check_cpp_nosql()
