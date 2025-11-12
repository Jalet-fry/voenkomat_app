-- Скрипт для проверки информации о базе данных в pgAdmin
-- Выполните этот запрос в Query Tool для базы voenkomat

-- ============================================
-- 1. ПРОВЕРКА ПАРАМЕТРОВ ПОДКЛЮЧЕНИЯ
-- ============================================
SELECT 
    'Параметры подключения' as info_type,
    current_database() as database_name,
    current_user as username,
    inet_server_addr() as server_host,
    inet_server_port() as server_port,
    version() as postgresql_version;

-- ============================================
-- 2. ПРОВЕРКА КОЛИЧЕСТВА ДАННЫХ В ТАБЛИЦАХ
-- ============================================
SELECT 
    'comissar' as table_name, 
    COUNT(*) as row_count,
    'Таблица комиссаров' as description
FROM comissar
UNION ALL
SELECT 'kategoria_godnosti', COUNT(*), 'Категории годности' FROM kategoria_godnosti
UNION ALL
SELECT 'med_osvidetelstvovanie', COUNT(*), 'Медосвидетельствования' FROM med_osvidetelstvovanie
UNION ALL
SELECT 'prizivnik', COUNT(*), 'Призывники' FROM prizivnik
UNION ALL
SELECT 'prizivnik_comissar', COUNT(*), 'Связь призывник-комиссар' FROM prizivnik_comissar
UNION ALL
SELECT 'prizivnik_meropriyatie', COUNT(*), 'Связь призывник-мероприятие' FROM prizivnik_meropriyatie
UNION ALL
SELECT 'prizivnoe_meropriyatie', COUNT(*), 'Призывные мероприятия' FROM prizivnoe_meropriyatie
UNION ALL
SELECT 'voenno_uchetnaya_karta', COUNT(*), 'Военно-учётные карты' FROM voenno_uchetnaya_karta
UNION ALL
SELECT 'voennyi_bilet', COUNT(*), 'Военные билеты' FROM voennyi_bilet
ORDER BY table_name;

-- ============================================
-- 3. ПРОВЕРКА СТРУКТУРЫ ТАБЛИЦ
-- ============================================
SELECT 
    table_name,
    (SELECT COUNT(*) 
     FROM information_schema.columns 
     WHERE table_name = t.table_name 
     AND table_schema = 'public') as column_count
FROM information_schema.tables t
WHERE table_schema = 'public' 
    AND table_type = 'BASE TABLE'
ORDER BY table_name;

-- ============================================
-- 4. ПРОВЕРКА СВЯЗЕЙ (FOREIGN KEYS)
-- ============================================
SELECT
    tc.table_name as source_table, 
    kcu.column_name as source_column, 
    ccu.table_name AS target_table,
    ccu.column_name AS target_column 
FROM information_schema.table_constraints AS tc 
JOIN information_schema.key_column_usage AS kcu
    ON tc.constraint_name = kcu.constraint_name
    AND tc.table_schema = kcu.table_schema
JOIN information_schema.constraint_column_usage AS ccu
    ON ccu.constraint_name = tc.constraint_name
    AND ccu.table_schema = tc.table_schema
WHERE tc.constraint_type = 'FOREIGN KEY' 
    AND tc.table_schema = 'public'
ORDER BY tc.table_name, kcu.column_name;

-- ============================================
-- 5. ПРИМЕРЫ ДАННЫХ ИЗ КЛЮЧЕВЫХ ТАБЛИЦ
-- ============================================
-- Призывники
SELECT 'Пример данных: Призывники' as info;
SELECT id_prizivnik, fio, data_rozhdeniya, nomer_pasporta 
FROM prizivnik 
LIMIT 5;

-- Комиссары
SELECT 'Пример данных: Комиссары' as info;
SELECT id_comissar, fio, dolzhnost, stazh_raboty 
FROM comissar 
LIMIT 5;

-- Категории годности
SELECT 'Пример данных: Категории годности' as info;
SELECT id_kategorii, nazvanie_kategorii, opisanie_ogranichenii 
FROM kategoria_godnosti 
LIMIT 5;

-- ============================================
-- 6. ПРОВЕРКА ЦЕЛОСТНОСТИ ДАННЫХ
-- ============================================
-- Проверка, что у всех призывников есть военные билеты
SELECT 
    'Проверка связей: Призывники без военных билетов' as check_name,
    COUNT(*) as problem_count
FROM prizivnik p
LEFT JOIN voennyi_bilet vb ON p.id_voennogo_bileta = vb.id_bileta
WHERE vb.id_bileta IS NULL;

-- Проверка, что у всех призывников есть военно-учётные карты
SELECT 
    'Проверка связей: Призывники без военно-учётных карт' as check_name,
    COUNT(*) as problem_count
FROM prizivnik p
LEFT JOIN voenno_uchetnaya_karta vuk ON p.id_voenno_uchetnoi_karty = vuk.id_karty
WHERE vuk.id_karty IS NULL;

-- ============================================
-- 7. ИНФОРМАЦИЯ О РАСПОЛОЖЕНИИ БД
-- ============================================
SELECT 
    'Расположение данных' as info_type,
    setting as data_directory
FROM pg_settings 
WHERE name = 'data_directory';

