# berkeleydb_manager.py
import shutil
import sys
import json
import os
import subprocess
import uuid
import zipfile
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QTableView, QVBoxLayout, QHBoxLayout,
                             QWidget, QMenuBar, QMenu, QStatusBar, QComboBox, QPushButton,
                             QDialog, QLabel, QLineEdit, QDialogButtonBox, QMessageBox,
                             QFormLayout, QTextEdit, QInputDialog, QHeaderView, QProgressDialog)
from PyQt5.QtCore import Qt, QAbstractTableModel
from PyQt5.QtGui import QKeySequence
# Импортируем конвертер
from berkeley.convertor import (PostgresToBerkeleyConverter, BERKELEY_DIR, DBSQL_PATH)
import requests

API_BASE_URL = "http://127.0.0.1:8000"
API_URL = f"{API_BASE_URL}/api"

# Пароль суперадминистратора для создания бэкапа
ADMIN_PASSWORD = "admin28"

# УДАЛЕНО старое COLUMN_TRANSLATIONS

# Константы для BerkeleyDB - ПЕРЕРАБОТАНЫ ДЛЯ ШКОЛЫ
BERKELEY_COLUMN_TRANSLATIONS = {
    'key': 'Ключ',
    'value': 'Значение (JSON)',
    'id': 'ID',
    'fullname': 'ФИО',
    'birthdate': 'Дата рождения',
    'address': 'Адрес',
    'phone': 'Телефон',
    'qualification': 'Квалификация',
    'experience': 'Стаж',
    'dayofweek': 'День недели',
    'time': 'Время',
    'homework': 'Домашнее задание',
    'type': 'Тип',
    'name': 'Название',
    'year': 'Год',
    'numberofstudents': 'Количество учеников',
    'hours': 'Часы',
    'specialization': 'Специализация',
    'number': 'Номер',
    'capacity': 'Вместимость',
    'responsible': 'Ответственный',
    'grade': 'Оценка',
    'date': 'Дата',
    'attendance': 'Посещаемость',
    'teacher_id': 'ID учителя',
    'room_id': 'ID кабинета',
    'class_id': 'ID класса',
    'student_id': 'ID ученика',
    'lesson_id': 'ID урока',
    'subject_id': 'ID предмета'
}

# ПЕРЕРАБОТАНЫ ДЛЯ ШКОЛЫ
BERKELEY_TABLE_TRANSLATIONS = {
    "lesson": "Уроки",
    "teacher": "Учителя",
    "student_lesson": "Ученики на уроках",
    "subject": "Предметы",
    "student": "Ученики",
    "room": "Кабинеты",
    "classwork": "Работа в классе",
    "class": "Классы"
}

# В файле berkeleydb_manager.py измените SPECIAL_QUERIES на:
SPECIAL_QUERIES = {
    "Ученики с классами": "/special/students_with_classes",
    "Учителя с предметами": "/special/teachers_with_subjects",
    "Оценки учеников": "/special/student_grades",
    "Посещаемость учеников": "/special/student_attendance_summary",
    "Информация о классах": "/special/class_info",
    "Информация о кабинетах": "/special/rooms_info",
}

# ОБНОВЛЕНЫ ЧИСЛОВЫЕ ПОЛЯ ДЛЯ ШКОЛЫ
NUMERIC_FIELDS = {'experience', 'hours', 'year', 'numberofstudents', 'capacity', 'grade'}
ID_FIELDS = {'id', 'teacher_id', 'room_id', 'class_id', 'student_id', 'lesson_id', 'subject_id'}

from PyQt5.QtWidgets import QShortcut
from PyQt5.QtGui import QKeySequence


class PasswordDialog(QDialog):
    """Диалог для ввода пароля"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Аутентификация администратора")
        self.setModal(True)
        self.resize(400, 200)
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout(self)

        # Информация
        info_label = QLabel(
            "Для создания резервной копии требуется\n"
            "авторизация суперадминистратора.\n\n"
            "Введите пароль:"
        )
        info_label.setAlignment(Qt.AlignCenter)
        layout.addWidget(info_label)

        # Поле для пароля
        self.password_input = QLineEdit()
        self.password_input.setEchoMode(QLineEdit.Password)  # Скрываем пароль
        self.password_input.setPlaceholderText("Введите пароль...")
        layout.addWidget(self.password_input)

        # Кнопки
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

        # Фокус на поле ввода пароля
        self.password_input.setFocus()

    def get_password(self):
        return self.password_input.text().strip()


class BaseRecordDialog(QDialog):
    """Базовый класс для диалогов работы с записями"""

    STYLES = {
        'add': """
            QDialog { background-color: #f0f8ff; }
            QLabel { font-weight: bold; color: #2c3e50; }
            QLineEdit { 
                padding: 8px; border: 2px solid #bdc3c7; border-radius: 5px; 
                background-color: white; font-size: 12px; 
            }
            QLineEdit:focus { border-color: #3498db; }
            QPushButton { 
                background-color: #3498db; color: white; border: none; 
                padding: 8px 15px; border-radius: 5px; font-weight: bold; 
            }
            QPushButton:hover { background-color: #2980b9; }
        """,
        'delete': """
            QDialog { background-color: #fff0f0; }
            QLabel { font-weight: bold; color: #c0392b; }
            QLineEdit { 
                padding: 8px; border: 2px solid #e74c3c; border-radius: 5px; 
                background-color: white; 
            }
            QPushButton { 
                background-color: #e74c3c; color: white; border: none; 
                padding: 8px 15px; border-radius: 5px; font-weight: bold; 
            }
            QPushButton:hover { background-color: #c0392b; }
        """,
        'query': """
            QDialog { background-color: #f8f9fa; }
            QComboBox, QTextEdit { 
                border: 2px solid #3498db; border-radius: 5px; 
                padding: 5px; background-color: white; 
            }
            QPushButton { 
                background-color: #9b59b6; color: white; border: none; 
                padding: 8px 15px; border-radius: 5px; font-weight: bold; 
            }
            QPushButton:hover { background-color: #8e44ad; }
        """,
        'backup': """
            QDialog { background-color: #e8f6f3; }
            QLineEdit { 
                padding: 8px; border: 2px solid #16a085; border-radius: 5px; 
                background-color: white; 
            }
            QPushButton { 
                background-color: #16a085; color: white; border: none; 
                padding: 8px 15px; border-radius: 5px; font-weight: bold; 
            }
            QPushButton:hover { background-color: #1abc9c; }
        """,
        'password': """
            QDialog { background-color: #fff8e1; }
            QLabel { font-weight: bold; color: #f57c00; font-size: 14px; }
            QLineEdit { 
                padding: 10px; border: 2px solid #ffb74d; border-radius: 5px; 
                background-color: white; font-size: 14px; 
            }
            QPushButton { 
                background-color: #ff9800; color: white; border: none; 
                padding: 10px 20px; border-radius: 5px; font-weight: bold; font-size: 14px;
            }
            QPushButton:hover { background-color: #f57c00; }
        """
    }

    def __init__(self, table_name, parent=None, style_type='add'):
        super().__init__(parent)
        self.table_name = table_name

        # 🔥 КРИТИЧНО
        self.fields = {}
        self.layout = QFormLayout(self)
        self.setLayout(self.layout)

        self.setStyleSheet(self.STYLES.get(style_type, ''))
        self.setModal(True)

    def get_russian_label(self, column_name):
        """Получить русское название для поля"""
        label = BERKELEY_COLUMN_TRANSLATIONS.get(column_name, column_name)
        # Добавляем подсказки для специальных полей
        if column_name in ['birthdate', 'date']:
            label += " (ГГГГ-ММ-ДД)"
        elif column_name == 'time':
            label += " (ЧЧ:ММ:СС)"
        elif column_name == 'grade':
            label += " (1-10)"
        return label

    def get_placeholder(self, column_name):
        """Получить placeholder для поля"""
        if column_name in NUMERIC_FIELDS or column_name in ID_FIELDS:
            return "Введите число"
        elif column_name in ['birthdate', 'date']:
            return "ГГГГ-ММ-ДД"
        elif column_name == 'time':
            return "ЧЧ:ММ:СС"
        elif column_name == 'attendance':
            return "true/false"
        return ""

    def convert_value(self, field_name, value):
        """Конвертировать значение в правильный тип"""
        if not value:
            return value

        if field_name in NUMERIC_FIELDS:
            try:
                return float(value) if '.' in value else int(value)
            except ValueError:
                return value
        elif field_name in ID_FIELDS:
            try:
                return int(value)
            except ValueError:
                return value
        elif field_name == 'attendance':
            if value.lower() in ['true', 'да', '1', 'yes']:
                return True
            elif value.lower() in ['false', 'нет', '0', 'no']:
                return False
        return value


class AddRecordDialog(BaseRecordDialog):
    def __init__(self, table_name, parent=None):
        super().__init__(table_name, parent, 'add')
        self.setWindowTitle(f"Добавить запись в таблицу: {BERKELEY_TABLE_TRANSLATIONS.get(table_name, table_name)}")
        self.resize(450, 400)
        self.init_ui()

    def init_ui(self):
        # Получаем поля по данным BerkeleyDB
        schema = self.get_table_schema()
        if not schema:
            QMessageBox.warning(self, "Ошибка", "Не удалось определить структуру таблицы")
            self.reject()
            return

        for column in schema["columns"]:
            edit = QLineEdit()
            edit.setPlaceholderText(self.get_placeholder(column))
            self.fields[column] = edit
            label = BERKELEY_COLUMN_TRANSLATIONS.get(column, column)
            self.layout.addRow(f"{label}:", edit)

        # Кнопки
        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        self.layout.addRow(buttons)

        # Фокус на первое поле
        if self.fields:
            list(self.fields.values())[0].setFocus()

    def get_table_schema(self):
        """
        В BerkeleyDB нет схемы — берём поля из первой записи
        """
        try:
            data, error = BerkeleyDBManager.fetch_table_data(self.table_name)
            if error or not data:
                return None
            return {"columns": list(data[0].keys())}
        except:
            return None

    def get_data(self):
        data = {}
        for name, widget in self.fields.items():
            value = widget.text().strip()
            if value:
                data[name] = self.convert_value(name, value)
        return data


class UpdateRecordDialog(AddRecordDialog):
    def __init__(self, table_name, record_id, parent=None):
        super().__init__(table_name, parent)
        self.record_id = record_id
        self.load_current_data()

    def load_current_data(self):
        """Заполнить поля текущими данными записи"""
        data, error = BerkeleyDBManager.fetch_table_data(self.table_name)
        if error or not data:
            QMessageBox.critical(self, "Ошибка", "Не удалось загрузить данные записи")
            return

        record = next((r for r in data if r.get("id") == self.record_id), None)
        if not record:
            QMessageBox.warning(self, "Ошибка", "Запись не найдена")
            return

        for field, widget in self.fields.items():
            if field in record:
                widget.setText(str(record[field]))


class BerkeleyTableModel(QAbstractTableModel):
    def __init__(self, data=None):
        super().__init__()
        self._data = data or []
        self._headers = []
        if data:
            self.set_data(data)

    def set_data(self, data):
        self.beginResetModel()
        self._data = data or []
        if data:
            # ИСПРАВЛЕНИЕ: Сохраняем порядок полей из первой записи
            if self._data and isinstance(self._data[0], dict):
                # Берем порядок полей из первой записи
                first_record_keys = list(self._data[0].keys())

                # Собираем все уникальные ключи из всех записей
                all_keys = set()
                for record in self._data:
                    if isinstance(record, dict):
                        all_keys.update(record.keys())

                # Сохраняем порядок из первой записи, добавляем остальные в конце
                ordered_headers = []
                # Сначала добавляем поля из первой записи в их оригинальном порядке
                for key in first_record_keys:
                    if key in all_keys:
                        ordered_headers.append(key)
                        all_keys.remove(key)

                # Затем добавляем оставшиеся поля в алфавитном порядке
                if all_keys:
                    ordered_headers.extend(sorted(list(all_keys)))

                self._headers = ordered_headers
            else:
                # Если нет данных или первая запись не словарь, используем старую логику
                all_keys = set()
                for record in self._data:
                    if isinstance(record, dict):
                        all_keys.update(record.keys())
                self._headers = sorted(list(all_keys))
        else:
            self._headers = []
        self.endResetModel()

    def rowCount(self, parent=None):
        return len(self._data)

    def columnCount(self, parent=None):
        return len(self._headers) if self._headers else 0

    def data(self, index, role=Qt.DisplayRole):
        if role == Qt.DisplayRole:
            row = index.row()
            col = index.column()
            if row < len(self._data) and col < len(self._headers):
                key = self._headers[col]
                value = self._data[row].get(key, '')
                return str(value)
        elif role == Qt.TextAlignmentRole:
            return Qt.AlignCenter
        return None

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.DisplayRole:
            if orientation == Qt.Horizontal:
                if section < len(self._headers):
                    header = self._headers[section]
                    return BERKELEY_COLUMN_TRANSLATIONS.get(header, header)
            else:
                return str(section + 1)
        return None


class ConvertDialog(QDialog):
    """Диалог конвертации PostgreSQL -> BerkeleyDB"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Конвертация PostgreSQL → BerkeleyDB")
        self.setModal(True)
        self.resize(500, 300)
        self.converter = PostgresToBerkeleyConverter()
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout(self)

        # Информация о конвертации
        info_label = QLabel(
            "Конвертация данных из PostgreSQL в BerkeleyDB\n\n"
            "Процесс включает:\n"
            "• Подключение к PostgreSQL\n"
            "• Получение структуры таблиц\n"
            "• Конвертацию данных в JSON формат\n"
            "• Создание баз данных BerkeleyDB\n\n"
            f"Базы данных будут сохранены в папке: {BERKELEY_DIR}"
        )
        info_label.setWordWrap(True)
        layout.addWidget(info_label)

        # Прогресс бар
        self.progress_bar = QProgressDialog("Подготовка к конвертации...", "Отмена", 0, 100, self)
        self.progress_bar.setWindowTitle("Конвертация")
        self.progress_bar.setWindowModality(Qt.WindowModal)
        self.progress_bar.canceled.connect(self.cancel_conversion)

        # Кнопки
        button_layout = QHBoxLayout()

        self.convert_btn = QPushButton("Начать конвертацию")
        self.convert_btn.clicked.connect(self.start_conversion)
        button_layout.addWidget(self.convert_btn)

        self.close_btn = QPushButton("Закрыть")
        self.close_btn.clicked.connect(self.reject)
        button_layout.addWidget(self.close_btn)

        layout.addLayout(button_layout)

    def update_progress(self, value, message):
        """Обновление прогресса конвертации"""
        self.progress_bar.setValue(value)
        self.progress_bar.setLabelText(message)
        QApplication.processEvents()

    def start_conversion(self):
        """Запуск процесса конвертации"""
        self.convert_btn.setEnabled(False)

        try:
            # Подключаемся к PostgreSQL
            success, message = self.converter.connect_postgres()
            if not success:
                QMessageBox.critical(self, "Ошибка", message)
                self.convert_btn.setEnabled(True)
                return

            # Запускаем конвертацию
            self.progress_bar.show()

            def progress_callback(value, msg):
                self.update_progress(value, msg)

            successful_tables, failed_tables = self.converter.convert_all_tables(progress_callback)

            # Показываем результаты
            self.show_results(successful_tables, failed_tables)

        except Exception as e:
            QMessageBox.critical(self, "Ошибка", f"Критическая ошибка при конвертации: {e}")
        finally:
            self.converter.close_connections()
            self.convert_btn.setEnabled(True)
            self.progress_bar.hide()

    def show_results(self, successful_tables, failed_tables):
        """Показать результаты конвертации"""
        result_text = f"Конвертация завершена!\n\n"

        if successful_tables:
            result_text += f"✅ Успешно сконвертировано: {len(successful_tables)} таблиц\n"
            result_text += f"Таблицы: {', '.join(successful_tables)}\n\n"

        if failed_tables:
            result_text += f"❌ Ошибки при конвертации: {len(failed_tables)} таблиц\n"
            for table_name, error in failed_tables:
                result_text += f"• {table_name}: {error}\n"

        result_text += f"\nБазы данных сохранены в папке: {BERKELEY_DIR}"

        QMessageBox.information(self, "Результаты конвертации", result_text)

    def cancel_conversion(self):
        """Отмена конвертации"""
        self.converter.close_connections()


class BerkeleyDBManager:

    @staticmethod
    def add_record(table_name: str, data: dict):
        r = requests.post(
            f"{API_URL}/{table_name}",
            json=data,
            timeout=10
        )
        if r.status_code in (200, 201):
            return True, "OK"
        return False, r.text

    @staticmethod
    def execute_special_query(endpoint: str):
        try:
            r = requests.get(f"{API_BASE_URL}{endpoint}", timeout=20)
            if r.status_code != 200:
                return None, r.text

            payload = r.json()
            if not payload.get("success"):
                return None, payload.get("error", "Unknown error")

            return payload.get("data", []), None

        except Exception as e:
            return None, str(e)
    @staticmethod
    def create_backup(backup_path=None, include_timestamp=True):
        """
        Создает zip-архив с резервной копией всех баз данных BerkeleyDB

        Args:
            backup_path (str): Путь для сохранения бэкапа. Если None, будет создан в папке backups
            include_timestamp (bool): Добавлять временную метку в имя файла

        Returns:
            tuple: (success, message, backup_file_path)
        """
        try:
            # Проверяем существование директории с базами данных
            if not os.path.exists(BERKELEY_DIR):
                return False, f"Директория {BERKELEY_DIR} не существует", None

            # Получаем список файлов .db
            db_files = []
            for filename in os.listdir(BERKELEY_DIR):
                if filename.endswith('.db'):
                    db_files.append(filename)

            if not db_files:
                return False, "Нет файлов баз данных для резервного копирования", None

            # Определяем путь для бэкапа
            if backup_path is None:
                # Создаем папку backups, если её нет
                backup_dir = os.path.join(os.path.dirname(BERKELEY_DIR), "backups")
                os.makedirs(backup_dir, exist_ok=True)

                # Формируем имя файла
                if include_timestamp:
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    backup_filename = f"berkeleydb_backup_{timestamp}.zip"
                else:
                    backup_filename = "berkeleydb_backup.zip"

                backup_path = os.path.join(backup_dir, backup_filename)

            # Создаем zip-архив
            with zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                # Добавляем файлы баз данных
                for db_file in db_files:
                    full_path = os.path.join(BERKELEY_DIR, db_file)
                    # Сохраняем структуру папок в архиве
                    arcname = os.path.join("berkeley_db", db_file)
                    zipf.write(full_path, arcname)

                # Добавляем информацию о бэкапе
                backup_info = {
                    "backup_date": datetime.now().isoformat(),
                    "files_count": len(db_files),
                    "files": db_files,
                    "berkeley_dir": BERKELEY_DIR,
                    "total_size": sum(os.path.getsize(os.path.join(BERKELEY_DIR, f)) for f in db_files)
                }

                # Создаем файл с информацией о бэкапе
                info_content = json.dumps(backup_info, indent=2, ensure_ascii=False)
                zipf.writestr("backup_info.json", info_content)

            # Проверяем, что архив создан успешно
            if os.path.exists(backup_path):
                file_size = os.path.getsize(backup_path)
                return True, f"Бэкап создан успешно: {backup_path} ({file_size} байт)", backup_path
            else:
                return False, "Не удалось создать файл бэкапа", None

        except PermissionError:
            return False, "Ошибка доступа к файлам. Проверьте права доступа", None
        except Exception as e:
            return False, f"Ошибка при создании бэкапа: {str(e)}", None

    @staticmethod
    def restore_backup(backup_path, restore_dir=None, overwrite=False):
        """
        Восстанавливает базы данных из zip-архива

        Args:
            backup_path (str): Путь к файлу бэкапа
            restore_dir (str): Директория для восстановления. Если None, используется BERKELEY_DIR
            overwrite (bool): Перезаписывать существующие файлы

        Returns:
            tuple: (success, message, restored_files)
        """
        try:
            # Проверяем существование файла бэкапа
            if not os.path.exists(backup_path):
                return False, f"Файл бэкапа не найден: {backup_path}", []

            # Определяем директорию для восстановления
            if restore_dir is None:
                restore_dir = BERKELEY_DIR

            # Создаем директорию, если её нет
            os.makedirs(restore_dir, exist_ok=True)

            # Извлекаем архив
            restored_files = []
            with zipfile.ZipFile(backup_path, 'r') as zipf:
                # Читаем информацию о бэкапе
                backup_info = None
                if 'backup_info.json' in zipf.namelist():
                    with zipf.open('backup_info.json') as info_file:
                        backup_info = json.load(info_file)

                # Извлекаем файлы
                for file_info in zipf.infolist():
                    # Пропускаем файл с информацией
                    if file_info.filename == 'backup_info.json':
                        continue

                    # Определяем путь для извлечения
                    # Убираем berkeley_db/ из пути, если оно есть
                    filename = os.path.basename(file_info.filename)
                    if 'berkeley_db/' in file_info.filename:
                        filename = file_info.filename.replace('berkeley_db/', '')

                    target_path = os.path.join(restore_dir, filename)

                    # Проверяем, существует ли файл
                    if os.path.exists(target_path) and not overwrite:
                        return False, f"Файл {filename} уже существует. Используйте overwrite=True", []

                    # Извлекаем файл
                    zipf.extract(file_info.filename, restore_dir)

                    # Переименовываем, если нужно
                    extracted_path = os.path.join(restore_dir, file_info.filename)
                    if extracted_path != target_path:
                        shutil.move(extracted_path, target_path)

                    restored_files.append(filename)

            return True, f"Восстановлено {len(restored_files)} файлов", restored_files

        except zipfile.BadZipFile:
            return False, "Некорректный zip-архив", []
        except Exception as e:
            return False, f"Ошибка при восстановлении: {str(e)}", []

    @staticmethod
    def list_backups(backup_dir=None):
        """
        Возвращает список доступных бэкапов

        Args:
            backup_dir (str): Директория с бэкапами

        Returns:
            list: Список информации о бэкапах
        """
        if backup_dir is None:
            backup_dir = os.path.join(os.path.dirname(BERKELEY_DIR), "backups")

        if not os.path.exists(backup_dir):
            return []

        backups = []
        for filename in sorted(os.listdir(backup_dir), reverse=True):
            if filename.endswith('.zip'):
                filepath = os.path.join(backup_dir, filename)
                file_info = {
                    'filename': filename,
                    'path': filepath,
                    'size': os.path.getsize(filepath),
                    'modified': datetime.fromtimestamp(os.path.getmtime(filepath)).isoformat()
                }

                # Пытаемся получить дополнительную информацию из архива
                try:
                    with zipfile.ZipFile(filepath, 'r') as zipf:
                        if 'backup_info.json' in zipf.namelist():
                            with zipf.open('backup_info.json') as info_file:
                                backup_info = json.load(info_file)
                                file_info['backup_date'] = backup_info.get('backup_date')
                                file_info['files_count'] = backup_info.get('files_count')
                except:
                    pass

                backups.append(file_info)

        return backups

    @staticmethod
    def update_record(table_name: str, record_id, data: dict):
        r = requests.put(
            f"{API_URL}/{table_name}/{record_id}",
            json=data,
            timeout=10
        )
        if r.status_code == 200:
            return True, "OK"
        return False, r.text

    @staticmethod
    def fetch_table_data(table_name, filters=None):
        """Получить данные таблицы BerkeleyDB из реальных файлов"""
        try:
            # Проверяем существование файла Berkeley DB
            db_path = os.path.join(BERKELEY_DIR, f"{table_name}.db")
            if not os.path.exists(db_path):
                error_msg = f"Файл {table_name}.db не найден в папке {BERKELEY_DIR}"
                print(f"ERROR: {error_msg}")
                return [], error_msg

            print(f"DEBUG: Чтение данных из {db_path}")
            print(f"DEBUG: Размер файла: {os.path.getsize(db_path)} байт")

            # Формируем SQL запрос с учетом фильтров
            sql_query = f"SELECT * FROM {table_name}"

            # В BerkeleyDB SQLite сложно делать сложные фильтры,
            # поэтому мы будем фильтровать на стороне клиента
            # Но можно попробовать простые фильтры если нужно

            sql_query += ";"

            # Используем dbsql для чтения данных из Berkeley DB
            temp_sql_file = os.path.join(BERKELEY_DIR, f"temp_read_{table_name}.sql")
            with open(temp_sql_file, 'w', encoding='utf-8') as f:
                f.write(sql_query)

            # Выполняем команду dbsql с перенаправлением ввода
            cmd = f'"{DBSQL_PATH}" "{db_path}" < "{temp_sql_file}"'
            print(f"DEBUG: Выполняем команду: {cmd}")

            try:
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True,
                                        encoding='utf-8', errors='ignore', timeout=30)

                print(f"DEBUG: Код возврата: {result.returncode}")
                print(f"DEBUG: STDOUT длина: {len(result.stdout)} символов")
                print(f"DEBUG: STDERR: {result.stderr}")

                # Удаляем временный файл
                if os.path.exists(temp_sql_file):
                    os.remove(temp_sql_file)

                if result.returncode != 0:
                    error_msg = f"Ошибка чтения из {table_name}.db: {result.stderr}"
                    print(f"ERROR: {error_msg}")
                    return [], error_msg

                # Парсим вывод dbsql
                data = BerkeleyDBManager.parse_dbsql_output(result.stdout, table_name)

                # Применяем фильтры на стороне клиента
                if filters and data:
                    data = BerkeleyDBManager.apply_filters(data, filters)

                print(f"DEBUG: После фильтрации: {len(data)} записей")
                return data, None

            except subprocess.TimeoutExpired:
                if os.path.exists(temp_sql_file):
                    os.remove(temp_sql_file)
                error_msg = f"Таймаут при чтении из {table_name}.db"
                print(f"ERROR: {error_msg}")
                return [], error_msg
            except Exception as e:
                if os.path.exists(temp_sql_file):
                    os.remove(temp_sql_file)
                error_msg = f"Ошибка выполнения dbsql: {str(e)}"
                print(f"ERROR: {error_msg}")
                return [], error_msg

        except Exception as e:
            error_msg = f"Ошибка BerkeleyDB: {str(e)}"
            print(f"ERROR: {error_msg}")
            return [], error_msg

    @staticmethod
    def apply_filters(data, filters):
        """Применяет фильтры к данным"""
        if not filters or not data:
            return data

        filtered_data = []

        for record in data:
            match = True

            for filter_key, filter_value in filters.items():
                # Пропускаем специальный фильтр поиска по всем колонкам
                if filter_key == "_search_all":
                    continue

                if filter_key in record:
                    record_value = str(record[filter_key]).lower()
                    filter_val = str(filter_value).lower()

                    # Простой поиск подстроки
                    if filter_val not in record_value:
                        match = False
                        break
                else:
                    match = False
                    break

            if match:
                filtered_data.append(record)

        return filtered_data

    @staticmethod
    def parse_dbsql_output(output, table_name):
        """Парсит вывод команды dbsql в структурированные данные - ИСПРАВЛЕННАЯ ВЕРСИЯ"""
        data = []
        lines = output.strip().split('\n')

        print(f"DEBUG: Парсинг вывода для {table_name}")
        print(f"DEBUG: Количество строк: {len(lines)}")

        # Ищем строки с данными (формат: ключ|JSON_значение)
        for i, line in enumerate(lines):
            line = line.strip()
            if not line or line.startswith('---'):
                continue

            # Пропускаем строку с заголовками
            if 'key' in line.lower() and 'value' in line.lower():
                continue

            print(f"DEBUG: Строка {i}: {line}")

            # Разделяем строку по первому символу '|'
            if '|' in line:
                parts = line.split('|', 1)  # Разделяем только на 2 части
                if len(parts) == 2:
                    key = parts[0].strip()
                    value_str = parts[1].strip()

                    print(f"DEBUG: Найдена запись - ключ: '{key}', значение: '{value_str}'")

                    # Парсим JSON значение
                    try:
                        if value_str and value_str.startswith('{') and value_str.endswith('}'):
                            json_value = json.loads(value_str)

                            # ИСПРАВЛЕНИЕ: Создаем запись ТОЛЬКО с полями из JSON
                            # НЕ добавляем поле 'key', чтобы избежать дублирования
                            record_data = {}

                            # Добавляем все поля из JSON
                            if isinstance(json_value, dict):
                                for json_key, json_val in json_value.items():
                                    record_data[json_key] = json_val

                            data.append(record_data)
                            print(f"DEBUG: Успешно распарсена запись с id {record_data.get('id', 'unknown')}")

                        else:
                            # Если это не JSON, сохраняем как есть с ключом
                            data.append({'key': key, 'value': value_str})
                            print(f"DEBUG: Не JSON значение, сохранено как есть")

                    except json.JSONDecodeError as e:
                        print(f"DEBUG: Ошибка парсинга JSON для строки '{value_str}': {e}")
                        data.append({'key': key, 'value': value_str, 'parse_error': str(e)})
                    except Exception as e:
                        print(f"DEBUG: Неожиданная ошибка: {e}")
                        data.append({'key': key, 'value': value_str, 'error': str(e)})

        print(f"DEBUG: Найдено записей после парсинга: {len(data)}")
        if data:
            print(f"DEBUG: Пример первой записи: {data[0]}")
        return data

    @staticmethod
    def get_available_tables():
        """Получить список доступных таблиц Berkeley DB только из marketplace_db"""
        available_tables = []
        if os.path.exists(BERKELEY_DIR):
            for filename in os.listdir(BERKELEY_DIR):
                if filename.endswith('.db'):
                    table_name = filename[:-3]  # Убираем .db
                    # Показываем только таблицы из нашей схемы
                    if table_name in BERKELEY_TABLE_TRANSLATIONS:
                        available_tables.append(table_name)
        return available_tables


class SpecialQueryDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Специальные запросы")
        self.resize(400, 200)

        layout = QVBoxLayout(self)

        self.combo = QComboBox()
        for name in SPECIAL_QUERIES.keys():
            self.combo.addItem(name)
        layout.addWidget(QLabel("Выберите запрос:"))
        layout.addWidget(self.combo)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addWidget(buttons)

    def selected_query(self):
        name = self.combo.currentText()
        return SPECIAL_QUERIES[name]


class BerkeleyDBWindow(QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.dialog_open = False
        self.active_table = "student"  # Изменено на student как первую таблицу
        self.last_query_result = None
        self.db_manager = BerkeleyDBManager()
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Система управления Школой - BerkeleyDB База данных")
        self.setGeometry(100, 100, 1200, 800)
        self.setup_styles()
        self.setup_ui()
        self.setup_shortcuts()
        self.update_table_list()
        self.on_table_changed()

    def setup_styles(self):
        """Настройка стилей приложения"""
        self.setStyleSheet("""
            QMainWindow { background-color: #ecf0f1; }
            QWidget { font-family: Arial, sans-serif; font-size: 12px; }
            QMenuBar {
                background-color: #3498db; color: white; font-weight: bold; padding: 5px;
            }
            QMenuBar::item {
                background-color: #3498db; color: white; padding: 5px 10px;
            }
            QMenuBar::item:selected { background-color: #2980b9; }
            QMenu { background-color: white; border: 1px solid #bdc3c7; }
            QMenu::item { padding: 5px 20px; }
            QMenu::item:selected { background-color: #3498db; color: white; }
            QStatusBar { background-color: #34495e; color: white; font-weight: bold; }
            QLabel { font-weight: bold; color: #2c3e50; font-size: 12px; }
            QComboBox {
                padding: 5px; border: 2px solid #bdc3c7; border-radius: 5px; 
                background-color: white; color: black; min-width: 120px;
            }
            QComboBox:selected{
                padding: 5px; border: 2px solid #bdc3c7; border-radius: 5px; 
                background-color: #3498db; color: white; min-width: 120px;
            }
            QComboBox:hover { border-color: black; color: black }
            QLineEdit {
                padding: 6px; border: 2px solid #bdc3c7; border-radius: 5px; 
                background-color: white;
            }
            QLineEdit:focus { border-color: #3498db; }
            QPushButton {
                background-color: #3498db; color: white; border: none; 
                padding: 8px 15px; border-radius: 5px; font-weight: bold; font-size: 12px;
            }
            QPushButton:hover { background-color: #2980b9; }
            QPushButton:pressed { background-color: #21618c; }
            QTableView {
                background-color: white; alternate-background-color: #f8f9fa;
                selection-background-color: #3498db; gridline-color: #bdc3c7; font-size: 15px;
            }
            QHeaderView::section {
                background-color: #34495e; color: white; padding: 6px; 
                border: 1px solid #2c3e50; font-weight: bold;
            }
        """)

    def setup_ui(self):
        """Настройка пользовательского интерфейса"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        layout = QVBoxLayout(central_widget)
        layout.setSpacing(10)
        layout.setContentsMargins(15, 15, 15, 15)

        self.create_menu()
        self.setup_control_panel(layout)
        self.setup_table(layout)
        self.setup_status_bar()

    def test_berkeley_connection(self):
        """Тестовый метод для проверки соединения с BerkeleyDB"""
        # Проверяем доступные таблицы
        available_tables = self.db_manager.get_available_tables()

        result_text = f"Доступные таблицы: {available_tables}\n\n"

        if not available_tables:
            result_text += "❌ Нет доступных таблиц\n"
            # Проверим папку BerkeleyDB
            if os.path.exists(BERKELEY_DIR):
                files = os.listdir(BERKELEY_DIR)
                db_files = [f for f in files if f.endswith('.db')]
                result_text += f"Файлы в {BERKELEY_DIR}:\n"
                for f in db_files:
                    file_path = os.path.join(BERKELEY_DIR, f)
                    size = os.path.getsize(file_path)
                    result_text += f"  • {f} ({size} байт)\n"
            else:
                result_text += f"Папка {BERKELEY_DIR} не существует\n"
        else:
            # Тестируем первую таблицу
            table_name = available_tables[0]
            result_text += f"Тестируем таблицу: {table_name}\n"

            # Проверяем существование файла
            db_path = os.path.join(BERKELEY_DIR, f"{table_name}.db")
            result_text += f"Путь к файлу: {db_path}\n"
            result_text += f"Файл существует: {os.path.exists(db_path)}\n"
            if os.path.exists(db_path):
                result_text += f"Размер файла: {os.path.getsize(db_path)} байт\n"

            # Пробуем прочитать данные
            data, error = self.db_manager.fetch_table_data(table_name)

            if error:
                result_text += f"❌ Ошибка: {error}\n"
            else:
                result_text += f"✅ Успешно!\n"
                result_text += f"Найдено записей: {len(data)}\n"

                if data:
                    result_text += f"Структура данных: {list(data[0].keys()) if data else 'Нет данных'}\n"
                    if len(data) > 0:
                        result_text += f"Первая запись: {data[0]}\n"

        QMessageBox.information(self, "Диагностика BerkeleyDB", result_text)

    def setup_control_panel(self, layout):
        """Настройка панели управления"""
        control_layout = QHBoxLayout()

        # Выбор таблицы BerkeleyDB
        control_layout.addWidget(QLabel("Активная таблица BerkeleyDB:"))
        self.table_combo = QComboBox()
        self.table_combo.currentIndexChanged.connect(self.on_table_changed)
        control_layout.addWidget(self.table_combo)

        # Фильтрация
        control_layout.addWidget(QLabel("Фильтр по:"))
        self.filter_combo = QComboBox()
        control_layout.addWidget(self.filter_combo)

        self.filter_value = QLineEdit()
        self.filter_value.setPlaceholderText("Введите значение для фильтрации...")
        control_layout.addWidget(self.filter_value)

        self.filter_button = QPushButton("Применить фильтр")
        self.filter_button.clicked.connect(self.apply_filter)
        control_layout.addWidget(self.filter_button)

        # Кнопка сброса фильтра
        self.clear_filter_button = QPushButton("Сбросить фильтр")
        self.clear_filter_button.clicked.connect(self.clear_filter)
        self.clear_filter_button.setStyleSheet("""
            QPushButton { background-color: #e74c3c; }
            QPushButton:hover { background-color: #c0392b; }
        """)
        control_layout.addWidget(self.clear_filter_button)

        control_layout.addStretch()
        layout.addLayout(control_layout)

    def update_table_list(self):
        """Обновить список доступных таблиц"""
        self.table_combo.clear()

        available_tables = self.db_manager.get_available_tables()

        if available_tables:
            for table_name in available_tables:
                display_name = BERKELEY_TABLE_TRANSLATIONS.get(table_name, table_name)
                self.table_combo.addItem(display_name, table_name)

            # Выбираем первую таблицу
            if available_tables:
                self.active_table = available_tables[0]
                self.on_table_changed()
        else:
            self.table_combo.addItem("Нет доступных таблиц", "")
            self.status_bar.showMessage("Нет доступных таблиц Berkeley DB")

    def back_to_postgres(self):
        """Вернуться к PostgreSQL"""
        self.close()
        if self.parent():
            self.parent().show()

    def setup_table(self, layout):
        """Настройка таблицы"""
        self.table_view = QTableView()
        self.model = BerkeleyTableModel()
        self.table_view.setModel(self.model)
        self.table_view.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table_view.setAlternatingRowColors(True)
        self.table_view.setSelectionBehavior(QTableView.SelectRows)
        self.table_view.setSortingEnabled(True)
        layout.addWidget(self.table_view)

    def setup_status_bar(self):
        """Настройка статус бара"""
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("BerkeleyDB система готова к работе")

    def create_backup(self):
        """Создать бэкап баз данных BerkeleyDB с проверкой пароля"""
        try:
            # Проверяем, не открыт ли уже другой диалог
            if self.dialog_open:
                return

            self.dialog_open = True

            try:
                # Создаем диалог для ввода пароля
                password_dialog = PasswordDialog(self)

                # Показываем диалог и ждем результата
                if password_dialog.exec_() != QDialog.Accepted:
                    # Пользователь нажал "Отмена"
                    self.status_bar.showMessage("Создание бэкапа отменено")
                    return

                # Получаем введенный пароль
                entered_password = password_dialog.get_password()

                # Проверяем пароль
                if entered_password != ADMIN_PASSWORD:
                    QMessageBox.critical(
                        self,
                        "Ошибка авторизации",
                        "Неверный пароль! Бэкап не создан.\n\n"
                        "Доступ разрешен только суперадминистратору."
                    )
                    self.status_bar.showMessage("Неверный пароль - бэкап не создан")
                    return

                # Пароль верный - создаем бэкап
                backup_dir = os.path.join(os.path.dirname(BERKELEY_DIR), "backups")
                os.makedirs(backup_dir, exist_ok=True)

                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                default_filename = f"berkeleydb_backup_{timestamp}.zip"
                default_path = os.path.join(backup_dir, default_filename)

                # Создаем бэкап используя статический метод класса
                success, message, backup_path = BerkeleyDBManager.create_backup(default_path, True)

                if success:
                    QMessageBox.information(
                        self,
                        "Бэкап создан",
                        f"✅ Авторизация прошла успешно!\n\n"
                        f"Резервная копия создана:\n{backup_path}\n\n"
                        f"{message}"
                    )
                    self.status_bar.showMessage(f"Бэкап создан: {os.path.basename(backup_path)}")
                else:
                    QMessageBox.critical(
                        self,
                        "Ошибка создания бэкапа",
                        f"✅ Авторизация прошла успешно!\n\n"
                        f"Но не удалось создать резервную копию:\n{message}"
                    )

            finally:
                self.dialog_open = False

        except Exception as e:
            self.dialog_open = False
            QMessageBox.critical(
                self,
                "Ошибка",
                f"Произошла ошибка при создании бэкапа:\n{str(e)}"
            )
    def create_menu(self):
        """Создание меню"""
        menubar = self.menuBar()

        # Меню Файл
        file_menu = menubar.addMenu("&Файл")

        file_menu.addSeparator()  # Разделитель

        exit_action = file_menu.addAction("&Выход")
        exit_action.setShortcut("Ctrl+E")
        exit_action.triggered.connect(self.close)

        # Меню Операции
        operations_menu = menubar.addMenu("&Операции")
        operations_data = [
            ("&Просмотр", "Ctrl+V", self.view_table),
            ("&Добавить", "Ctrl+A", self.add_record),
            ("&Обновить", "Ctrl+U", self.update_record),
            ("&Удалить", "Ctrl+D", self.delete_record),
            ("Создать &бэкап (требуется пароль)", "Ctrl+B", self.create_backup),  # <-- ЗДЕСЬ
            ("&Запросы", "Ctrl+Q", self.show_queries),
            ("&Сохранить результат", "Ctrl+S", self.save_query_result),
            ("&Конвертировать данные", "Ctrl+C", self.convert_data)
        ]

        for text, shortcut, handler in operations_data:
            action = operations_menu.addAction(text)
            action.setShortcut(shortcut)
            action.triggered.connect(handler)

        # Также можно добавить пункт для восстановления из бэкапа
        restore_action = file_menu.addAction("Восстановить из &бэкапа")
        restore_action.triggered.connect(self.restore_from_backup)

    def restore_from_backup(self):
        """Восстановить базы данных из бэкапа"""
        try:
            # Сначала проверяем пароль для восстановления
            password_dialog = PasswordDialog(self)
            password_dialog.setWindowTitle("Аутентификация для восстановления")

            if password_dialog.exec_() != QDialog.Accepted:
                self.status_bar.showMessage("Восстановление отменено")
                return

            entered_password = password_dialog.get_password()

            if entered_password != ADMIN_PASSWORD:
                QMessageBox.critical(
                    self,
                    "Ошибка авторизации",
                    "Неверный пароль! Восстановление отменено.\n\n"
                    "Доступ разрешен только суперадминистратору."
                )
                self.status_bar.showMessage("Неверный пароль - восстановление отменено")
                return

            # Пароль верный - продолжаем восстановление
            # Получаем список доступных бэкапов
            backup_dir = os.path.join(os.path.dirname(BERKELEY_DIR), "backups")

            if not os.path.exists(backup_dir):
                QMessageBox.warning(
                    self,
                    "Нет бэкапов",
                    f"✅ Авторизация прошла успешно!\n\n"
                    f"Но директория с бэкапами не найдена:\n{backup_dir}"
                )
                return

            backups = BerkeleyDBManager.list_backups(backup_dir)

            if not backups:
                QMessageBox.information(
                    self,
                    "Нет бэкапов",
                    "✅ Авторизация прошла успешно!\n\n"
                    "Но нет доступных резервных копий для восстановления."
                )
                return

            # Создаем диалог выбора бэкапа
            backup_names = []
            for backup in backups:
                filename = backup['filename']
                size_mb = backup['size'] / (1024 * 1024)
                modified = backup.get('modified', '')
                backup_date = backup.get('backup_date', '')

                display_text = f"{filename}"
                if backup_date:
                    display_text += f" (создан: {backup_date})"
                display_text += f" - {size_mb:.2f} MB"

                backup_names.append(display_text)

            # Показываем диалог выбора
            selected_backup, ok = QInputDialog.getItem(
                self,
                "Выберите бэкап для восстановления",
                "✅ Авторизация успешна! Доступные бэкапы:",
                backup_names,
                0,
                False
            )

            if not ok or not selected_backup:
                return

            # Находим выбранный бэкап
            selected_filename = selected_backup.split(" (")[0] if " (" in selected_backup else \
                selected_backup.split(" - ")[0]
            selected_path = os.path.join(backup_dir, selected_filename)

            # Подтверждение восстановления
            reply = QMessageBox.question(
                self,
                "Подтверждение восстановления",
                f"✅ Авторизация прошла успешно!\n\n"
                f"Вы уверены, что хотите восстановить базы данных из бэкапа?\n\n"
                f"Файл: {selected_filename}\n\n"
                f"ВНИМАНИЕ: Существующие данные будут перезаписаны!",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No
            )

            if reply != QMessageBox.Yes:
                return

            # Выполняем восстановление
            success, message, restored_files = BerkeleyDBManager.restore_backup(
                selected_path,
                None,  # использовать стандартную директорию
                True  # перезаписать существующие файлы
            )

            if success:
                QMessageBox.information(
                    self,
                    "Восстановление завершено",
                    f"✅ Авторизация прошла успешно!\n\n"
                    f"Базы данных успешно восстановлены из бэкапа:\n\n"
                    f"Файл: {selected_filename}\n"
                    f"Восстановлено файлов: {len(restored_files)}\n"
                    f"{message}"
                )

                # Обновляем список таблиц
                self.update_table_list()
                self.status_bar.showMessage(f"Базы данных восстановлены из {selected_filename}")
            else:
                QMessageBox.critical(
                    self,
                    "Ошибка восстановления",
                    f"✅ Авторизация прошла успешно!\n\n"
                    f"Но не удалось восстановить базы данных:\n{message}"
                )

        except Exception as e:
            QMessageBox.critical(
                self,
                "Ошибка",
                f"Произошла ошибка при восстановлении:\n{str(e)}"
            )

    def setup_shortcuts(self):
        """Настройка горячих клавиш"""
        # Alt+D — открыть выбор таблицы
        shortcut_alt_d = QShortcut(QKeySequence("Alt+D"), self)
        shortcut_alt_d.activated.connect(self.open_table_menu)


    def open_table_menu(self):
        """Открывает меню выбора таблицы BerkeleyDB"""
        if self.table_combo.count() > 0:
            self.table_combo.showPopup()

    def set_active_table(self, table_name):
        """Установить активную таблицу"""
        self.active_table = table_name
        index = self.table_combo.findData(table_name)
        if index >= 0:
            self.table_combo.setCurrentIndex(index)

    def on_table_changed(self, index=0):
        """Обработчик изменения таблицы"""
        if self.table_combo.count() > 0 and self.table_combo.currentData():
            self.active_table = self.table_combo.currentData()
            self.update_filter_columns()
            self.view_table()

    def clear_filter(self):
        """Сбросить фильтр"""
        self.filter_value.clear()
        self.view_table()

    def update_filter_columns(self):
        """Обновить список колонок для фильтрации"""
        self.filter_combo.clear()

        # Добавляем опцию "Все колонки" для поиска по всем полям
        self.filter_combo.addItem("Все колонки", "all_columns")

        # Загружаем данные, чтобы определить доступные колонки
        data, error = self.db_manager.fetch_table_data(self.active_table)
        if data and not error:
            if data:
                columns = list(data[0].keys())
                # Сортируем колонки для удобства
                sorted_columns = sorted(columns)
                for column in sorted_columns:
                    display_name = BERKELEY_COLUMN_TRANSLATIONS.get(column, column)
                    self.filter_combo.addItem(display_name, column)

    def apply_filter(self):
        """Применить фильтр с улучшенной логикой"""
        self.view_table()

    def view_table(self):
        """Просмотр активной таблицы с улучшенной фильтрацией"""
        if not self.active_table:
            QMessageBox.information(self, "Информация", "Выберите таблицу для просмотра")
            return

        try:
            filters = {}
            filter_column = self.filter_combo.currentData()
            filter_value = self.filter_value.text().strip()

            if filter_value:  # Если есть значение для фильтрации
                if filter_column == "all_columns":
                    # Поиск по всем колонкам
                    filters["_search_all"] = filter_value
                else:
                    # Фильтрация по конкретной колонке
                    filters[filter_column] = filter_value

            data, error = self.db_manager.fetch_table_data(self.active_table, filters)

            if error:
                self.status_bar.showMessage(f"Ошибка: {error}")
                QMessageBox.critical(self, "Ошибка", error)
            else:
                # Применяем фильтрацию на стороне клиента если нужно
                if filters and "_search_all" in filters:
                    data = self.apply_search_all_filter(data, filters["_search_all"])

                self.model.set_data(data)
                record_count = len(data)
                table_name = self.table_combo.currentText()

                if filter_value:
                    self.status_bar.showMessage(
                        f"BerkeleyDB '{table_name}': найдено {record_count} записей "
                        f"(фильтр: '{filter_value}')"
                    )
                else:
                    self.status_bar.showMessage(f"BerkeleyDB '{table_name}': загружено {record_count} записей")

                self.last_query_result = data

        except Exception as e:
            error_msg = f"Неожиданная ошибка: {str(e)}"
            self.status_bar.showMessage(error_msg)
            QMessageBox.critical(self, "Ошибка", error_msg)

    def apply_search_all_filter(self, data, search_term):
        """Фильтрует данные по всем колонкам"""
        if not search_term:
            return data

        search_term_lower = search_term.lower()
        filtered_data = []

        for record in data:
            # Проверяем каждое поле записи
            for key, value in record.items():
                if search_term_lower in str(value).lower():
                    filtered_data.append(record)
                    break  # Переходим к следующей записи если нашли совпадение

        return filtered_data

    def add_record(self):
        if self.dialog_open:
            return

        self.dialog_open = True
        dialog = AddRecordDialog(self.active_table, self)

        try:
            if dialog.exec_() == QDialog.Accepted:
                data = dialog.get_data()
                ok, msg = self.db_manager.add_record(self.active_table, data)
                if ok:
                    self.view_table()
                else:
                    QMessageBox.critical(self, "Ошибка", msg)
        finally:
            self.dialog_open = False

    def update_record(self):
        indexes = self.table_view.selectionModel().selectedRows()
        if not indexes:
            QMessageBox.warning(self, "Ошибка", "Выберите запись")
            return

        row = indexes[0].row()
        record = self.model._data[row]
        record_id = record.get("id")

        if record_id is None:
            QMessageBox.warning(self, "Ошибка", "Нет поля id")
            return

        dialog = UpdateRecordDialog(self.active_table, record_id, self)
        if dialog.exec_() == QDialog.Accepted:
            data = dialog.get_data()
            ok, msg = self.db_manager.update_record(
                self.active_table, record_id, data
            )
            if ok:
                self.view_table()
            else:
                QMessageBox.critical(self, "Ошибка", msg)

    def delete_record(self):
        indexes = self.table_view.selectionModel().selectedRows()
        if not indexes:
            QMessageBox.warning(self, "Ошибка", "Выберите строку")
            return

        row = indexes[0].row()
        record = self.model._data[row]
        key = record.get("id")

        if key is None:
            QMessageBox.warning(self, "Ошибка", "Нет id")
            return

        reply = QMessageBox.question(
            self,
            "Удалить",
            f"Удалить запись с id={key}?",
            QMessageBox.Yes | QMessageBox.No
        )

        if reply != QMessageBox.Yes:
            return

        r = requests.delete(f"{API_URL}/{self.active_table}/{key}")

        if r.status_code == 200:
            self.view_table()
            QMessageBox.information(self, "OK", "Запись удалена")
        else:
            QMessageBox.critical(self, "Ошибка", r.text)

    def show_queries(self):
        dialog = SpecialQueryDialog(self)
        if dialog.exec_() != QDialog.Accepted:
            return

        endpoint = dialog.selected_query()

        data, error = self.db_manager.execute_special_query(endpoint)

        if error:
            QMessageBox.critical(self, "Ошибка запроса", error)
            return

        if not data:
            QMessageBox.information(self, "Результат", "Запрос вернул пустой результат")
            return

        # 🔥 показываем результат в таблице
        self.model.set_data(data)
        self.last_query_result = data

        self.status_bar.showMessage(
            f"Выполнен специальный запрос: {len(data)} записей"
        )

    def save_query_result(self):
        """Сохранить результат последнего запроса (заглушка)"""
        QMessageBox.information(self, "Информация",
                                "Функция сохранения результатов будет реализована позже")



def main():
    """Запуск автономного приложения BerkeleyDB"""
    app = QApplication(sys.argv)
    app.setStyle('Fusion')

    window = BerkeleyDBWindow()
    window.show()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()