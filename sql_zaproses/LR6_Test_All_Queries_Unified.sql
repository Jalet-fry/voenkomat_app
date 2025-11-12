-- Скрипт для автоматического тестирования всех запросов из LR6
-- ВСЕ РЕЗУЛЬТАТЫ собираются в одну таблицу и выводятся в конце
-- Это решение проблемы pgAdmin, который показывает только последний результат

-- Создаём временную таблицу для хранения всех результатов
CREATE TEMP TABLE IF NOT EXISTS all_query_results (
    query_number VARCHAR(20),
    query_description TEXT,
    result_data JSONB
);

-- Очищаем таблицу перед запуском
TRUNCATE TABLE all_query_results;

DO $$ BEGIN RAISE NOTICE '=== Начало выполнения всех запросов ==='; END $$;

-- Запрос 2.1.1: Вывести призывников с названием категории годности и возрастом
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.1 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.1',
    'Вывести призывников с названием категории годности и возрастом (в годах), отсортировать по возрасту (убыв.)',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', p.id_prizivnik,
            'fio', p.fio,
            'nomer_pasporta', p.nomer_pasporta,
            'kategoria_godnosti', kg.nazvanie_kategorii,
            'age', EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)),
            'data_rozhdeniya', p.data_rozhdeniya
        ) ORDER BY EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) DESC, p.fio
    )
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii;

-- Запрос 2.1.2: Посчитать число призывников по городам и показать только города с 1 призывником
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.2 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH city_counts AS (
    SELECT 
        TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
        COUNT(p.id_prizivnik) AS prizivniki_count
    FROM public.prizivnik p
    WHERE p.adres_prozhivaniya IS NOT NULL
    GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
    HAVING COUNT(p.id_prizivnik) = 1
)
SELECT 
    '2.1.2',
    'Посчитать число призывников по городам и показать только города с 1 призывником',
    jsonb_agg(
        jsonb_build_object(
            'city', city,
            'prizivniki_count', prizivniki_count
        ) ORDER BY prizivniki_count DESC
    )
FROM city_counts;

-- Запрос 2.1.3: Для каждого призывника показать, сколько медицинских освидетельствований связано с ним
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.3 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.3',
    'Для каждого призывника показать, сколько медицинских освидетельствований связано с ним',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', p.id_prizivnik,
            'fio', p.fio,
            'med_osvidetelstvovaniya_count', 
            (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik)
        ) ORDER BY (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik) DESC, p.fio
    )
FROM public.prizivnik p;

-- Запрос 2.1.4: Показать призывников с их военными билетами
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.4 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.4',
    'Показать призывников с их военными билетами (номер билета, воинское звание, категория)',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', p.id_prizivnik,
            'fio', p.fio,
            'data_rozhdeniya', p.data_rozhdeniya,
            'nomer_bileta', vb.nomer_bileta,
            'voinskoe_zvanie', vb.voinskoe_zvanie,
            'kategoria', vb.kategoria,
            'data_vydachi', vb.data_vydachi
        ) ORDER BY p.fio
    )
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik;

-- Запрос 2.1.5: Посчитать количество призывников по городу
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.5 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH city_counts AS (
    SELECT 
        TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
        COUNT(*) AS prizivniki_count
    FROM public.prizivnik p
    WHERE p.adres_prozhivaniya IS NOT NULL
    GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
)
SELECT 
    '2.1.5',
    'Посчитать количество призывников по городу',
    jsonb_agg(
        jsonb_build_object(
            'city', city,
            'prizivniki_count', prizivniki_count
        ) ORDER BY prizivniki_count DESC
    )
FROM city_counts;

-- Запрос 2.1.6: Выбрать призывников, у которых есть военный билет
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.6 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.6',
    'Выбрать призывников, у которых есть военный билет',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', p.id_prizivnik,
            'fio', p.fio,
            'data_rozhdeniya', p.data_rozhdeniya,
            'age', EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya))
        ) ORDER BY p.fio
    )
FROM public.prizivnik p
WHERE EXISTS (
    SELECT 1 FROM public.voennyi_bilet vb WHERE vb.id_prizivnika = p.id_prizivnik
);

-- Запрос 2.1.7: Для каждого комиссара показать количество связанных призывников
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.7 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH comissar_counts AS (
    SELECT 
        c.id_comissar,
        c.fio,
        c.dolzhnost,
        COUNT(pc.id_prizivnik) AS prizivniki_count
    FROM public.comissar c
    LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
    GROUP BY c.id_comissar, c.fio, c.dolzhnost
)
SELECT 
    '2.1.7',
    'Для каждого комиссара показать количество связанных призывников',
    jsonb_agg(
        jsonb_build_object(
            'id_comissar', id_comissar,
            'fio', fio,
            'dolzhnost', dolzhnost,
            'prizivniki_count', prizivniki_count
        ) ORDER BY prizivniki_count DESC, fio
    )
FROM comissar_counts;

-- Запрос 2.1.8: Топ-5 призывников по количеству связанных с ними мероприятий
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.8 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH prizivnik_counts AS (
    SELECT 
        p.id_prizivnik,
        p.fio,
        COALESCE(COUNT(pm.id_meropriyatiya), 0) AS meropriyatiya_count
    FROM public.prizivnik p
    LEFT JOIN public.prizivnik_meropriyatie pm ON pm.id_prizivnik = p.id_prizivnik
    GROUP BY p.id_prizivnik, p.fio
    HAVING COALESCE(COUNT(pm.id_meropriyatiya), 0) > 0
    ORDER BY COALESCE(COUNT(pm.id_meropriyatiya), 0) DESC
    LIMIT 5
)
SELECT 
    '2.1.8',
    'Топ-5 призывников по количеству связанных с ними мероприятий',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', id_prizivnik,
            'fio', fio,
            'meropriyatiya_count', meropriyatiya_count
        ) ORDER BY meropriyatiya_count DESC
    )
FROM prizivnik_counts;

-- Запрос 2.1.9: Найти призывников, не имеющих ни одного медицинского освидетельствования
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.9 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.9',
    'Найти призывников, не имеющих ни одного медицинского освидетельствования',
    jsonb_agg(
        jsonb_build_object(
            'id_prizivnik', p.id_prizivnik,
            'fio', p.fio,
            'data_rozhdeniya', p.data_rozhdeniya
        ) ORDER BY p.fio
    )
FROM public.prizivnik p
WHERE NOT EXISTS (
    SELECT 1 FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik
);

-- Запрос 2.1.10: Вывести комиссаров и количество призывников в каждом
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.10 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH comissar_counts AS (
    SELECT 
        c.id_comissar,
        c.fio,
        c.dolzhnost,
        c.stazh_raboty,
        COUNT(pc.id_prizivnik) AS prizivniki_count
    FROM public.comissar c
    LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
    GROUP BY c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
)
SELECT 
    '2.1.10',
    'Вывести комиссаров и количество призывников в каждом',
    jsonb_agg(
        jsonb_build_object(
            'id_comissar', id_comissar,
            'fio', fio,
            'dolzhnost', dolzhnost,
            'stazh_raboty', stazh_raboty,
            'prizivniki_count', prizivniki_count
        ) ORDER BY prizivniki_count DESC
    )
FROM comissar_counts;

-- Запрос 2.1.11: Показать комиссаров с должностью "Военный комиссар" и отсортировать по стажу
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.11 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.11',
    'Показать комиссаров с должностью "Военный комиссар" и отсортировать по стажу',
    jsonb_agg(
        jsonb_build_object(
            'id_comissar', id_comissar,
            'fio', fio,
            'dolzhnost', dolzhnost,
            'stazh_raboty', stazh_raboty,
            'kontaktnyi_telefon', kontaktnyi_telefon
        ) ORDER BY stazh_raboty DESC
    )
FROM public.comissar
WHERE dolzhnost = 'Военный комиссар';

-- Запрос 2.1.12: Найти комиссара(ов) с максимальным стажем работы
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.12 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.12',
    'Найти комиссара(ов) с максимальным стажем работы',
    jsonb_agg(
        jsonb_build_object(
            'id_comissar', c.id_comissar,
            'fio', c.fio,
            'dolzhnost', c.dolzhnost,
            'stazh_raboty', c.stazh_raboty
        )
    )
FROM public.comissar c
WHERE c.stazh_raboty = (SELECT MAX(stazh_raboty) FROM public.comissar);

-- Запрос 2.1.13: Показать военные билеты вместе с ФИО призывника и возрастом призывника
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.13 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.13',
    'Показать военные билеты вместе с ФИО призывника и возрастом призывника',
    jsonb_agg(
        jsonb_build_object(
            'id_bileta', vb.id_bileta,
            'nomer_bileta', vb.nomer_bileta,
            'voinskoe_zvanie', vb.voinskoe_zvanie,
            'kategoria', vb.kategoria,
            'prizivnik_name', p.fio,
            'prizivnik_age', EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)),
            'data_vydachi', vb.data_vydachi
        ) ORDER BY p.fio
    )
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika;

-- Запрос 2.1.14: Подсчитать количество военных билетов по категориям годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.14 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH kategoria_counts AS (
    SELECT 
        kategoria,
        COUNT(*) AS biletov_count
    FROM public.voennyi_bilet
    GROUP BY kategoria
)
SELECT 
    '2.1.14',
    'Подсчитать количество военных билетов по категориям годности',
    jsonb_agg(
        jsonb_build_object(
            'kategoria', kategoria,
            'biletov_count', biletov_count
        ) ORDER BY biletov_count DESC
    )
FROM kategoria_counts;

-- Запрос 2.1.15: Выбрать военные билеты призывников, родившихся до 2000 года
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.15 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
SELECT 
    '2.1.15',
    'Выбрать военные билеты призывников, родившихся до 2000 года',
    jsonb_agg(
        jsonb_build_object(
            'id_bileta', vb.id_bileta,
            'nomer_bileta', vb.nomer_bileta,
            'kategoria', vb.kategoria,
            'voinskoe_zvanie', vb.voinskoe_zvanie,
            'data_vydachi', vb.data_vydachi,
            'id_prizivnik', p.id_prizivnik
        ) ORDER BY p.data_rozhdeniya
    )
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
WHERE p.data_rozhdeniya < '2000-01-01';

-- Запрос 2.1.16: Топ-5 призывных мероприятий по дате проведения
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.16 ==='; END $$;
INSERT INTO all_query_results (query_number, query_description, result_data)
WITH top_meropriyatiya AS (
    SELECT 
        id_meropriyatiya,
        tip_meropriyatiya,
        data_provedeniya,
        mesto_provedeniya,
        fio_comissara
    FROM public.prizivnoe_meropriyatie
    ORDER BY data_provedeniya DESC
    LIMIT 5
)
SELECT 
    '2.1.16',
    'Топ-5 призывных мероприятий по дате проведения',
    jsonb_agg(
        jsonb_build_object(
            'id_meropriyatiya', id_meropriyatiya,
            'tip_meropriyatiya', tip_meropriyatiya,
            'data_provedeniya', data_provedeniya,
            'mesto_provedeniya', mesto_provedeniya,
            'fio_comissara', fio_comissara
        ) ORDER BY data_provedeniya DESC
    )
FROM top_meropriyatiya;

-- Выводим все результаты в виде читаемой таблицы
DO $$ BEGIN RAISE NOTICE '=== Все запросы выполнены успешно! ==='; END $$;
DO $$ BEGIN RAISE NOTICE '=== Результаты всех запросов собраны в таблице all_query_results ==='; END $$;

-- Финальный запрос: выводим все результаты
SELECT 
    query_number,
    query_description,
    COALESCE(jsonb_array_length(result_data), 0) AS rows_count,
    CASE 
        WHEN result_data IS NULL OR jsonb_array_length(result_data) = 0 THEN 'Нет результатов'
        ELSE 'Есть результаты (см. JSON в колонке result_data)'
    END AS status
FROM all_query_results
ORDER BY query_number;

-- Для просмотра конкретного запроса используйте:
-- SELECT jsonb_pretty(result_data) FROM all_query_results WHERE query_number = '2.1.1';

