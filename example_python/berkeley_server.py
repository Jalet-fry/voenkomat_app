import os
import json
import subprocess
from fastapi import FastAPI, HTTPException
from typing import Dict, Any, List

DBSQL_PATH = r"C:\Program Files (x86)\Oracle\Berkeley DB 12cR1 6.0.30\bin\dbsql.exe"
BERKELEY_DIR = "./berkeley_db"

app = FastAPI(title="BerkeleyDB API", version="1.0")


# -------------------------------------------------
# ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
# -------------------------------------------------

def db_path(table: str) -> str:
    path = os.path.join(BERKELEY_DIR, f"{table}.db")
    if not os.path.exists(path):
        raise HTTPException(404, detail=f"Database {table}.db not found")
    return path


def run_dbsql(dbfile: str, sql: str) -> str:
    temp_sql = os.path.join(BERKELEY_DIR, "temp.sql")
    with open(temp_sql, "w", encoding="utf-8") as f:
        f.write(sql)

    cmd = f'"{DBSQL_PATH}" "{dbfile}" < "{temp_sql}"'
    result = subprocess.run(
        cmd,
        shell=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="ignore"
    )

    if os.path.exists(temp_sql):
        os.remove(temp_sql)

    if result.returncode != 0:
        raise HTTPException(500, detail=result.stderr)

    return result.stdout


def parse_output(output: str) -> List[Dict[str, Any]]:
    data = []
    for line in output.splitlines():
        line = line.strip()
        if not line or "|" not in line:
            continue
        if "key" in line.lower() and "value" in line.lower():
            continue

        parts = line.split("|", 1)
        if len(parts) < 2:
            continue

        key, value = parts
        try:
            if value.strip().startswith("{") and value.strip().endswith("}"):
                record = json.loads(value.strip())
                # Добавляем ID из ключа если его нет в JSON
                if "id" not in record:
                    record["id"] = key.strip()
                data.append(record)
        except json.JSONDecodeError:
            # Если это не JSON, создаем простой объект
            record = {"id": key.strip(), "value": value.strip()}
            data.append(record)
        except Exception:
            pass
    return data


# -------------------------------------------------
# API
# -------------------------------------------------

@app.get("/api/{table}")
def get_all(table: str):
    dbfile = db_path(table)
    output = run_dbsql(dbfile, f"SELECT * FROM {table};")
    return parse_output(output)


@app.post("/api/{table}")
def insert_record(table: str, record: Dict[str, Any]):
    if "id" not in record:
        raise HTTPException(400, detail="Field 'id' is required")

    key = str(record["id"])
    value = json.dumps(record, ensure_ascii=False)

    dbfile = db_path(table)
    run_dbsql(
        dbfile,
        f"INSERT INTO {table} VALUES ('{key}', '{value}');"
    )

    return {"status": "created", "id": key}


@app.put("/api/{table}/{key}")
def update_record(table: str, key: str, record: Dict[str, Any]):
    record["id"] = int(key) if key.isdigit() else key
    value = json.dumps(record, ensure_ascii=False)

    dbfile = db_path(table)

    run_dbsql(dbfile, f"DELETE FROM {table} WHERE key='{key}';")
    run_dbsql(
        dbfile,
        f"INSERT INTO {table} VALUES ('{key}', '{value}');"
    )

    return {"status": "updated", "id": key}


@app.delete("/api/{table}/{key}")
def delete_record(table: str, key: str):
    dbfile = db_path(table)
    run_dbsql(dbfile, f"DELETE FROM {table} WHERE key='{key}';")
    return {"status": "deleted", "id": key}


# -------------------------------------------------
# ПРОСТЫЕ СПЕЦИАЛЬНЫЕ ЗАПРОСЫ ДЛЯ ШКОЛЫ
# -------------------------------------------------

@app.get("/special/students_with_classes")
def students_with_classes():
    """Ученики и их классы (только основные данные)"""
    try:
        db_students = parse_output(run_dbsql(db_path("student"), "SELECT * FROM student;"))
        db_classes = parse_output(run_dbsql(db_path("class"), "SELECT * FROM class;"))

        # Простой словарь классов
        classes_dict = {}
        for c in db_classes:
            class_id = c.get("id")
            if class_id:
                classes_dict[str(class_id)] = {
                    "class_name": c.get("name", ""),
                    "class_year": c.get("year", "")
                }

        result = []
        for student in db_students:
            student_id = student.get("id")
            class_id = student.get("class_id")

            # Только основные поля ученика
            student_data = {
                "id": student_id,
                "ФИО": student.get("fullname", ""),
                "Телефон": student.get("phone", ""),
                "ID_класса": class_id
            }

            # Добавляем информацию о классе
            if class_id and str(class_id) in classes_dict:
                class_info = classes_dict[str(class_id)]
                student_data["Класс"] = class_info["class_name"]
                student_data["Год_обучения"] = class_info["class_year"]

            result.append(student_data)

        return {"success": True, "data": result, "count": len(result)}
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


@app.get("/special/teachers_with_subjects")
def teachers_with_subjects():
    """Учителя и их предметы (только основные данные)"""
    try:
        db_teachers = parse_output(run_dbsql(db_path("teacher"), "SELECT * FROM teacher;"))
        db_subjects = parse_output(run_dbsql(db_path("subject"), "SELECT * FROM subject;"))

        result = []
        for teacher in db_teachers:
            teacher_id = teacher.get("id")

            # Находим предметы этого учителя
            teacher_subjects = []
            for subject in db_subjects:
                if subject.get("teacher_id") == teacher_id:
                    teacher_subjects.append(subject.get("name", ""))

            # Только основные поля
            teacher_data = {
                "id": teacher_id,
                "ФИО": teacher.get("fullname", ""),
                "Квалификация": teacher.get("qualification", ""),
                "Стаж": teacher.get("experience", ""),
                "Предметы": ", ".join(teacher_subjects) if teacher_subjects else "Нет предметов"
            }

            result.append(teacher_data)

        return {"success": True, "data": result, "count": len(result)}
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


@app.get("/special/lessons_schedule")
def lessons_schedule():
    """Расписание уроков (простое)"""
    try:
        db_lessons = parse_output(run_dbsql(db_path("lesson"), "SELECT * FROM lesson;"))
        db_teachers = parse_output(run_dbsql(db_path("teacher"), "SELECT * FROM teacher;"))
        db_rooms = parse_output(run_dbsql(db_path("room"), "SELECT * FROM room;"))

        # Словари для быстрого поиска
        teachers_dict = {}
        for t in db_teachers:
            teachers_dict[str(t.get("id"))] = t.get("fullname", "")

        rooms_dict = {}
        for r in db_rooms:
            rooms_dict[str(r.get("id"))] = r.get("number", "")

        result = []
        for lesson in db_lessons:
            lesson_id = lesson.get("id")
            teacher_id = lesson.get("teacher_id")
            room_id = lesson.get("room_id")

            # Только основные поля
            lesson_data = {
                "id": lesson_id,
                "День": lesson.get("dayofweek", ""),
                "Время": str(lesson.get("time", ""))[:5],  # Только часы:минуты
                "Тип_урока": lesson.get("type", ""),
                "Домашнее_задание": lesson.get("homework", "")[:50] + "..." if len(
                    lesson.get("homework", "")) > 50 else lesson.get("homework", "")
            }

            # Добавляем учителя если есть
            if teacher_id and str(teacher_id) in teachers_dict:
                lesson_data["Учитель"] = teachers_dict[str(teacher_id)]

            # Добавляем кабинет если есть
            if room_id and str(room_id) in rooms_dict:
                lesson_data["Кабинет"] = rooms_dict[str(room_id)]

            result.append(lesson_data)

        # Сортируем по дню и времени
        day_order = {"Понедельник": 1, "Вторник": 2, "Среда": 3, "Четверг": 4, "Пятница": 5}
        result.sort(key=lambda x: (day_order.get(x.get("День", ""), 99), x.get("Время", "")))

        return {"success": True, "data": result, "count": len(result)}
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


@app.get("/special/student_grades")
def student_grades():
    """Оценки учеников (простая таблица)"""
    try:
        db_classwork = parse_output(run_dbsql(db_path("classwork"), "SELECT * FROM classwork;"))
        db_students = parse_output(run_dbsql(db_path("student"), "SELECT * FROM student;"))
        db_lessons = parse_output(run_dbsql(db_path("lesson"), "SELECT * FROM lesson;"))

        # Словари для быстрого поиска
        students_dict = {}
        for s in db_students:
            students_dict[str(s.get("id"))] = s.get("fullname", "")

        lessons_dict = {}
        for l in db_lessons:
            lessons_dict[str(l.get("id"))] = {
                "day": l.get("dayofweek", ""),
                "time": str(l.get("time", ""))[:5]
            }

        result = []
        for work in db_classwork:
            grade = work.get("grade")
            if grade is None:  # Пропускаем если нет оценки
                continue

            student_id = work.get("student_id")
            lesson_id = work.get("lesson_id")

            # Только основные поля
            grade_data = {
                "Оценка": grade,
                "Тип_работы": work.get("type", ""),
                "Дата": work.get("date", "")
            }

            # Добавляем ученика если есть
            if student_id and str(student_id) in students_dict:
                grade_data["Ученик"] = students_dict[str(student_id)]
                grade_data["ID_ученика"] = student_id

            # Добавляем информацию об уроке если есть
            if lesson_id and str(lesson_id) in lessons_dict:
                lesson_info = lessons_dict[str(lesson_id)]
                grade_data["День_урока"] = lesson_info["day"]
                grade_data["Время_урока"] = lesson_info["time"]

            result.append(grade_data)

        # Статистика
        if result:
            grades = [g["Оценка"] for g in result if isinstance(g["Оценка"], (int, float))]
            avg_grade = sum(grades) / len(grades) if grades else 0

            return {
                "success": True,
                "data": result,
                "count": len(result),
                "statistics": {
                    "Всего_оценок": len(result),
                    "Средний_балл": round(avg_grade, 2),
                    "Макс_оценка": max(grades) if grades else 0,
                    "Мин_оценка": min(grades) if grades else 0
                }
            }
        else:
            return {"success": True, "data": [], "count": 0, "message": "Нет оценок"}

    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


@app.get("/special/student_attendance_summary")
def student_attendance_summary():
    """Посещаемость учеников (краткая статистика)"""
    try:
        db_classwork = parse_output(run_dbsql(db_path("classwork"), "SELECT * FROM classwork;"))
        db_students = parse_output(run_dbsql(db_path("student"), "SELECT * FROM student;"))

        # Группируем по ученикам
        attendance_by_student = {}
        for work in db_classwork:
            attendance = work.get("attendance")
            student_id = work.get("student_id")

            if attendance is not None and student_id is not None:
                if student_id not in attendance_by_student:
                    attendance_by_student[student_id] = {"present": 0, "total": 0}

                attendance_by_student[student_id]["total"] += 1
                if attendance is True:
                    attendance_by_student[student_id]["present"] += 1

        # Словарь студентов
        students_dict = {}
        for s in db_students:
            students_dict[str(s.get("id"))] = s.get("fullname", "")

        result = []
        for student_id, stats in attendance_by_student.items():
            present = stats["present"]
            total = stats["total"]
            attendance_rate = (present / total * 100) if total > 0 else 0

            student_data = {
                "ID_ученика": student_id,
                "ФИО": students_dict.get(str(student_id), f"Ученик {student_id}"),
                "Был_на": present,
                "Всего_уроков": total,
                "Процент_посещаемости": round(attendance_rate, 1)
            }

            result.append(student_data)

        # Общая статистика
        if result:
            total_present = sum(s["Был_на"] for s in result)
            total_lessons = sum(s["Всего_уроков"] for s in result)
            avg_attendance = (total_present / total_lessons * 100) if total_lessons > 0 else 0

            return {
                "success": True,
                "data": result,
                "count": len(result),
                "total_statistics": {
                    "Всего_учеников": len(result),
                    "Всего_уроков": total_lessons,
                    "Всего_посещений": total_present,
                    "Средняя_посещаемость": round(avg_attendance, 1)
                }
            }
        else:
            return {"success": True, "data": [], "count": 0, "message": "Нет данных о посещаемости"}

    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


@app.get("/special/class_info")
def class_info():
    """Информация о классах (простая)"""
    try:
        db_classes = parse_output(run_dbsql(db_path("class"), "SELECT * FROM class;"))
        db_students = parse_output(run_dbsql(db_path("student"), "SELECT * FROM student;"))

        # Считаем учеников в каждом классе
        students_by_class = {}
        for student in db_students:
            class_id = student.get("class_id")
            if class_id:
                if class_id not in students_by_class:
                    students_by_class[class_id] = 0
                students_by_class[class_id] += 1

        result = []
        for class_item in db_classes:
            class_id = class_item.get("id")
            student_count = students_by_class.get(class_id, 0)

            class_data = {
                "id": class_id,
                "Название": class_item.get("name", ""),
                "Год": class_item.get("year", ""),
                "Количество_учеников": student_count,
                "Тип": class_item.get("type", "")
            }

            result.append(class_data)

        return {
            "success": True,
            "data": result,
            "count": len(result),
            "total_students": sum(c["Количество_учеников"] for c in result)
        }
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")


# В файл fastapi_app.py добавьте этот endpoint:

@app.get("/special/lessons_with_teachers_rooms")
def lessons_with_teachers_rooms():
    """Уроки с учителями и кабинетами (простое расписание)"""
    try:
        db_lessons = parse_output(run_dbsql(db_path("lesson"), "SELECT * FROM lesson;"))
        db_teachers = parse_output(run_dbsql(db_path("teacher"), "SELECT * FROM teacher;"))
        db_rooms = parse_output(run_dbsql(db_path("room"), "SELECT * FROM room;"))

        # Простые словари
        teachers_dict = {}
        for t in db_teachers:
            teachers_dict[str(t.get("id"))] = t.get("fullname", "")

        rooms_dict = {}
        for r in db_rooms:
            rooms_dict[str(r.get("id"))] = {
                "number": r.get("number", ""),
                "type": r.get("type", "")
            }

        result = []
        for lesson in db_lessons:
            lesson_id = lesson.get("id")
            teacher_id = lesson.get("teacher_id")
            room_id = lesson.get("room_id")

            # Только основные поля
            lesson_data = {
                "id": lesson_id,
                "День": lesson.get("dayofweek", ""),
                "Время": str(lesson.get("time", ""))[:5],  # Только часы:минуты
                "Тип_урока": lesson.get("type", ""),
                "Домашнее_задание": lesson.get("homework", "")[:50] + "..." if len(
                    lesson.get("homework", "")) > 50 else lesson.get("homework", "")
            }

            # Добавляем учителя если есть
            if teacher_id and str(teacher_id) in teachers_dict:
                lesson_data["Учитель"] = teachers_dict[str(teacher_id)]
                lesson_data["ID_учителя"] = teacher_id

            # Добавляем кабинет если есть
            if room_id and str(room_id) in rooms_dict:
                room_info = rooms_dict[str(room_id)]
                lesson_data["Кабинет"] = room_info["number"]
                lesson_data["Тип_кабинета"] = room_info["type"]
                lesson_data["ID_кабинета"] = room_id

            result.append(lesson_data)

        # Сортируем по дню и времени
        day_order = {"Понедельник": 1, "Вторник": 2, "Среда": 3, "Четверг": 4, "Пятница": 5}
        result.sort(key=lambda x: (day_order.get(x.get("День", ""), 99), x.get("Время", "")))

        return {
            "success": True,
            "data": result,
            "count": len(result),
            "statistics": {
                "Всего_уроков": len(result),
                "С_учителем": sum(1 for l in result if "Учитель" in l),
                "С_кабинетом": sum(1 for l in result if "Кабинет" in l)
            }
        }
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")

@app.get("/special/rooms_info")
def rooms_info():
    """Информация о кабинетах (простая)"""
    try:
        db_rooms = parse_output(run_dbsql(db_path("room"), "SELECT * FROM room;"))
        db_lessons = parse_output(run_dbsql(db_path("lesson"), "SELECT * FROM lesson;"))

        # Считаем уроки в каждом кабинете
        lessons_by_room = {}
        for lesson in db_lessons:
            room_id = lesson.get("room_id")
            if room_id:
                if room_id not in lessons_by_room:
                    lessons_by_room[room_id] = 0
                lessons_by_room[room_id] += 1

        result = []
        for room in db_rooms:
            room_id = room.get("id")
            lesson_count = lessons_by_room.get(room_id, 0)

            room_data = {
                "id": room_id,
                "Номер": room.get("number", ""),
                "Тип": room.get("type", ""),
                "Вместимость": room.get("capacity", ""),
                "Ответственный": room.get("responsible", ""),
                "Количество_уроков": lesson_count
            }

            result.append(room_data)

        return {
            "success": True,
            "data": result,
            "count": len(result),
            "total_capacity": sum(r["Вместимость"] for r in result if isinstance(r["Вместимость"], (int, float)))
        }
    except Exception as e:
        raise HTTPException(500, detail=f"Ошибка: {str(e)}")

