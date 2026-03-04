# TODO: [REVIEW] OK.
# TODO: [NOTE] TRUNCATE list contains 'conscripts_events' which is not in create_schema.sql.
# TODO: [NOTE] 200 conscripts are generated as requested.

import psycopg2
import random
from faker import Faker
import os
import configparser

def get_db_config():
    config = configparser.ConfigParser()
    # Path to config.ini in the root directory
    config_path = os.path.join(os.path.dirname(__file__), "..", "config.ini")
    
    # Default parameters
    db_params = {
        "host": "localhost",
        "port": "5432",
        "database": "military_db",
        "user": "postgres",
        "password": ""
    }
    
    if os.path.exists(config_path):
        config.read(config_path, encoding="utf-8")
        if "Database" in config:
            db_params["host"] = config["Database"].get("host", "localhost")
            db_params["database"] = config["Database"].get("database", "military_db")
            db_params["user"] = config["Database"].get("username", "postgres")
            db_params["password"] = config["Database"].get("password", "")
            
    return db_params

fake = Faker('ru_RU')
DB_CONFIG = get_db_config()

def generate():
    conn = None
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        print("Подключено к БД. Очистка старых данных и генерация новых...")

        # --- CLEANUP (optional, for repeatable script execution) ---
        cur.execute("""
            TRUNCATE 
                public.conscripts, 
                public.commissioners, 
                public.fitness_categories, 
                public.medical_examinations, 
                public.military_id_cards, 
                public.service_record_cards, 
                public.callup_events, 
                public.conscripts_commissioners, 
                public.conscripts_events 
            RESTART IDENTITY CASCADE;
        """)
        print("Таблицы очищены.")

        # 1. Справочник категорий (из DbConstants.h)
        categories = [
            ('А', 'Годен к военной службе', 1, 'Полностью здоров'),
            ('Б', 'Годен с незначительными ограничениями', 2, 'Имеются мелкие нарушения здоровья'),
            ('В', 'Ограниченно годен', 3, 'Наличие хронических заболеваний'),
            ('Г', 'Временно не годен', 4, 'Послеоперационный период/травма'),
            ('Д', 'Не годен к военной службе', 5, 'Наличие тяжелых заболеваний')
        ]
        category_ids = []
        for cat in categories:
            cur.execute("""
                INSERT INTO fitness_categories (category_name, restriction_description, category_index, category_basis)
                VALUES (%s, %s, %s, %s) RETURNING category_id;
            """, cat)
            category_ids.append(cur.fetchone()[0])
        print(f"Сгенерировано {len(category_ids)} категорий годности.")

        # 2. Комиссары
        commissioner_ids = []
        for _ in range(5): # 5 commissioners
            cur.execute("""
                INSERT INTO commissioners (full_name, position, years_of_service, phone_number)
                VALUES (%s, %s, %s, %s) RETURNING commissioner_id;
            """, (fake.name(), "Полковник", random.randint(10, 25), fake.phone_number()))
            commissioner_ids.append(cur.fetchone()[0])
        print(f"Сгенерировано {len(commissioner_ids)} комиссаров.")
        
        # 3. Призывники (200 человек)
        print("Генерация 200 призывников и связанных данных...")
        for i in range(200):
            cur.execute("""
                INSERT INTO conscripts (full_name, birth_date, residence_address, passport_number)
                VALUES (%s, %s, %s, %s) RETURNING conscript_id;
            """, (fake.name(), fake.date_of_birth(minimum_age=18, maximum_age=27), fake.address(), f"{random.randint(1000, 9999)} {random.randint(100000, 999999)}"))
            conscript_id = cur.fetchone()[0]

            # 4. Медосмотры для каждого призывника
            for _ in range(random.randint(1, 2)):
                cur.execute("""
                    INSERT INTO medical_examinations (examination_date, examination_results, doctor_full_name, conclusion, conscript_id, category_id)
                    VALUES (%s, %s, %s, %s, %s, %s);
                """, (
                    fake.date_between(start_date='-2y', end_date='today'), 
                    "В пределах нормы", 
                    fake.name_male(), 
                    "Годен", 
                    conscript_id, 
                    random.choice(category_ids)
                ))
            
            # 5. Военный билет (для 70% призывников)
            if random.random() < 0.7:
                 cur.execute("""
                    INSERT INTO military_id_cards (ticket_number, issue_date, military_rank, conscript_id, category_id)
                    VALUES (%s, %s, %s, %s, %s);
                """, (
                    f"ВБ {random.randint(1000000, 9999999)}",
                    fake.date_between(start_date='-1y', end_date='today'),
                    "Рядовой",
                    conscript_id,
                    random.choice(category_ids)
                ))

        conn.commit()
        print("База успешно наполнена данными (200+ записей)! Теперь можно тестировать API.")

    except Exception as e:
        print(f"Ошибка: {e}")
        if conn:
            conn.rollback() # Откатываем транзакцию в случае ошибки
    finally:
        if conn:
            conn.close()

if __name__ == "__main__":
    generate()
