-- Скрипт создания структуры БД "Военкомат"
-- Соответствует DbConstants.h

DROP TABLE IF EXISTS public.conscripts_events CASCADE;
DROP TABLE IF EXISTS public.conscripts_commissioners CASCADE;
DROP TABLE IF EXISTS public.medical_examinations CASCADE;
DROP TABLE IF EXISTS public.military_id_cards CASCADE;
DROP TABLE IF EXISTS public.service_record_cards CASCADE;
DROP TABLE IF EXISTS public.callup_events CASCADE;
DROP TABLE IF EXISTS public.conscripts CASCADE;
DROP TABLE IF EXISTS public.commissioners CASCADE;
DROP TABLE IF EXISTS public.fitness_categories CASCADE;

-- 1. Категории годности
CREATE TABLE public.fitness_categories (
    category_id SERIAL PRIMARY KEY,
    category_name VARCHAR(10) NOT NULL,
    restriction_description TEXT,
    category_index INTEGER,
    category_basis TEXT
);

-- 2. Комиссары
CREATE TABLE public.commissioners (
    commissioner_id SERIAL PRIMARY KEY,
    full_name VARCHAR(255) NOT NULL,
    position VARCHAR(100),
    years_of_service INTEGER,
    phone_number VARCHAR(20)
);

-- 3. Призывники
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

-- 6. Учетные карты (Service Records)
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
    commissioner_full_name VARCHAR(255),
    commissioner_id INTEGER REFERENCES public.commissioners(commissioner_id)
);

-- 8. Связь Призывники-Комиссары (Many-to-Many)
CREATE TABLE public.conscripts_commissioners (
    conscript_id INTEGER REFERENCES public.conscripts(conscript_id) ON DELETE CASCADE,
    commissioner_id INTEGER REFERENCES public.commissioners(commissioner_id) ON DELETE CASCADE,
    interaction_date DATE,
    office_number VARCHAR(10),
    PRIMARY KEY (conscript_id, commissioner_id)
);
