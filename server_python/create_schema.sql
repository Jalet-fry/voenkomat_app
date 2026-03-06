-- Скрипт создания структуры БД "Военкомат"
-- Соответствует требованиям Лабораторной работы №1

DROP TABLE IF EXISTS public.conscripts_events CASCADE;
DROP TABLE IF EXISTS public.conscripts_commissioners CASCADE;
DROP TABLE IF EXISTS public.medical_examinations CASCADE;
DROP TABLE IF EXISTS public.military_id_cards CASCADE;
DROP TABLE IF EXISTS public.service_record_cards CASCADE;
DROP TABLE IF EXISTS public.callup_events CASCADE;
DROP TABLE IF EXISTS public.conscripts CASCADE;
DROP TABLE IF EXISTS public.commissioners CASCADE;
DROP TABLE IF EXISTS public.fitness_categories CASCADE;

-- 1. [LookUp] Категории годности
CREATE TABLE public.fitness_categories (
    category_id SERIAL PRIMARY KEY,
    category_name VARCHAR(10) NOT NULL,
    restriction_description TEXT,
    category_index INTEGER,
    category_basis TEXT
);

-- 2. [LookUp] Комиссары
CREATE TABLE public.commissioners (
    commissioner_id SERIAL PRIMARY KEY,
    full_name VARCHAR(255) NOT NULL,
    position VARCHAR(100),
    years_of_service INTEGER,
    phone_number VARCHAR(20)
);

-- 3. [Main] Призывники
CREATE TABLE public.conscripts (
    conscript_id SERIAL PRIMARY KEY,
    full_name VARCHAR(255) NOT NULL,
    birth_date DATE,
    residence_address TEXT,
    passport_number VARCHAR(20),
    military_ticket_id INTEGER,
    registration_card_id INTEGER
);

-- 4. Медосмотры
CREATE TABLE public.medical_examinations (
    certification_id SERIAL PRIMARY KEY,
    examination_date DATE,
    examination_results TEXT,
    doctor_full_name VARCHAR(255),
    conclusion TEXT,
    conscript_id INTEGER REFERENCES public.conscripts(conscript_id) ON DELETE CASCADE,
    category_id INTEGER REFERENCES public.fitness_categories(category_id)
);

-- 5. Военные билеты
CREATE TABLE public.military_id_cards (
    ticket_id SERIAL PRIMARY KEY,
    ticket_number VARCHAR(50),
    issue_date DATE,
    military_rank VARCHAR(50),
    category VARCHAR(10),
    conscript_id INTEGER REFERENCES public.conscripts(conscript_id) ON DELETE CASCADE,
    category_id INTEGER REFERENCES public.fitness_categories(category_id)
);

-- 6. Учетные карты
CREATE TABLE public.service_record_cards (
    card_id SERIAL PRIMARY KEY,
    card_number VARCHAR(50),
    registration_date DATE,
    deferment_history TEXT,
    military_specialty VARCHAR(100),
    conscript_id INTEGER REFERENCES public.conscripts(conscript_id) ON DELETE CASCADE
);

-- 7. События призыва
CREATE TABLE public.callup_events (
    event_id SERIAL PRIMARY KEY,
    event_type VARCHAR(100),
    event_datetime TIMESTAMP,
    event_location TEXT,
    commissioner_id INTEGER REFERENCES public.commissioners(commissioner_id)
);

-- 8. Связь Призывники-Комиссары (M2M)
CREATE TABLE public.conscripts_commissioners (
    conscript_id INTEGER REFERENCES public.conscripts(conscript_id) ON DELETE CASCADE,
    commissioner_id INTEGER REFERENCES public.commissioners(commissioner_id) ON DELETE CASCADE,
    interaction_date DATE,
    office_number VARCHAR(10),
    PRIMARY KEY (conscript_id, commissioner_id)
);

-- ==========================================
-- ПЕРВИЧНОЕ НАПОЛНЕНИЕ (СПРАВОЧНИКИ)
-- ==========================================

INSERT INTO public.fitness_categories (category_name, restriction_description, category_index) VALUES
('А', 'Годен к военной службе', 1),
('Б', 'Годен с незначительными ограничениями', 2),
('В', 'Ограниченно годен', 3),
('Г', 'Временно не годен', 4),
('Д', 'Не годен к военной службе', 5);

INSERT INTO public.commissioners (full_name, position, years_of_service) VALUES
('Иванов Иван Иванович', 'Главный комиссар', 20),
('Петров Петр Петрович', 'Заместитель комиссара', 15),
('Сидоров Сидор Сидорович', 'Старший инспектор', 10);

-- ==========================================
-- ТРИГГЕРЫ И ФУНКЦИИ (ЛАБОРАТОРНАЯ №1)
-- ==========================================

-- Функция для проверки даты рождения (не может быть в будущем)
CREATE OR REPLACE FUNCTION check_conscript_data() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.birth_date > CURRENT_DATE THEN
        RAISE EXCEPTION 'Дата рождения не может быть в будущем';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_check_conscript_birth
BEFORE INSERT OR UPDATE ON public.conscripts
FOR EACH ROW EXECUTE FUNCTION check_conscript_data();

-- Функция для логирования изменений в призывниках (пример оператора/функции)
CREATE TABLE IF NOT EXISTS public.audit_log (
    log_id SERIAL PRIMARY KEY,
    table_name VARCHAR(50),
    operation VARCHAR(10),
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    record_id INTEGER
);

CREATE OR REPLACE FUNCTION log_conscript_changes() RETURNS TRIGGER AS $$
BEGIN
    IF (TG_OP = 'INSERT') THEN
        INSERT INTO public.audit_log(table_name, operation, record_id) VALUES ('conscripts', 'INSERT', NEW.conscript_id);
    ELSIF (TG_OP = 'UPDATE') THEN
        INSERT INTO public.audit_log(table_name, operation, record_id) VALUES ('conscripts', 'UPDATE', NEW.conscript_id);
    ELSIF (TG_OP = 'DELETE') THEN
        INSERT INTO public.audit_log(table_name, operation, record_id) VALUES ('conscripts', 'DELETE', OLD.conscript_id);
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_log_conscripts
AFTER INSERT OR UPDATE OR DELETE ON public.conscripts
FOR EACH ROW EXECUTE FUNCTION log_conscript_changes();
