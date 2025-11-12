-- Скрипт для автоматического тестирования всех запросов из LR6
-- Выполняет запросы последовательно с паузой 10 секунд между ними
-- Останавливается при первой ошибке
-- ВНИМАНИЕ: pgAdmin показывает результаты только последнего запроса при выполнении скрипта целиком
-- Для просмотра результатов всех запросов выполняйте их по одному (выделите запрос и F5)

-- Запрос 2.1.1: Вывести призывников с названием категории годности и возрастом (в годах), отсортировать по возрасту (убыв.)
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.1 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    p.nomer_pasporta,
    kg.nazvanie_kategorii AS kategoria_godnosti,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age,
    p.data_rozhdeniya
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY age DESC, p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.2: Посчитать число призывников по городам и показать только города с 1 призывником
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.2 ==='; END $$;
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(p.id_prizivnik) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
HAVING COUNT(p.id_prizivnik) = 1
ORDER BY prizivniki_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.3: Для каждого призывника показать, сколько медицинских освидетельствований связано с ним
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.3 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik) AS med_osvidetelstvovaniya_count
FROM public.prizivnik p
ORDER BY med_osvidetelstvovaniya_count DESC, p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.4: Показать призывников с их военными билетами (номер билета, воинское звание, категория)
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.4 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    vb.data_vydachi
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.5: Посчитать количество призывников по городу
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.5 ==='; END $$;
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(*) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
ORDER BY prizivniki_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.6: Выбрать призывников, у которых есть военный билет
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.6 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age
FROM public.prizivnik p
WHERE EXISTS (
    SELECT 1 FROM public.voennyi_bilet vb WHERE vb.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.7: Для каждого комиссара показать количество связанных призывников
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.7 ==='; END $$;
SELECT
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY prizivniki_count DESC, c.fio;

-- select pg_sleep(5);

-- Запрос 2.1.8: Топ-5 призывников по количеству связанных с ними мероприятий
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.8 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    COALESCE(COUNT(pm.id_meropriyatiya), 0) AS meropriyatiya_count
FROM public.prizivnik p
LEFT JOIN public.prizivnik_meropriyatie pm 
    ON pm.id_prizivnik = p.id_prizivnik
GROUP BY p.id_prizivnik, p.fio
HAVING COALESCE(COUNT(pm.id_meropriyatiya), 0) > 0
ORDER BY meropriyatiya_count DESC
LIMIT 5;

-- select pg_sleep(5);

-- Запрос 2.1.9: Найти призывников, не имеющих ни одного медицинского освидетельствования
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.9 ==='; END $$;
SELECT p.id_prizivnik, p.fio, p.data_rozhdeniya
FROM public.prizivnik p
WHERE NOT EXISTS (
    SELECT 1 FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.10: Вывести комиссаров и количество призывников в каждом
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.10 ==='; END $$;
SELECT
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    c.stazh_raboty,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
ORDER BY prizivniki_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.11: Показать комиссаров с должностью "Военный комиссар" и отсортировать по стажу
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.11 ==='; END $$;
SELECT id_comissar, fio, dolzhnost, stazh_raboty, kontaktnyi_telefon
FROM public.comissar
WHERE dolzhnost = 'Военный комиссар'
ORDER BY stazh_raboty DESC;

-- select pg_sleep(5);

-- Запрос 2.1.12: Найти комиссара(ов) с максимальным стажем работы
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.12 ==='; END $$;
SELECT c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
FROM public.comissar c
WHERE c.stazh_raboty = (SELECT MAX(stazh_raboty) FROM public.comissar);

-- select pg_sleep(5);

-- Запрос 2.1.13: Показать военные билеты вместе с ФИО призывника и возрастом призывника
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.13 ==='; END $$;
SELECT
    vb.id_bileta,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    p.fio AS prizivnik_name,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS prizivnik_age,
    vb.data_vydachi
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.14: Подсчитать количество военных билетов по категориям годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.14 ==='; END $$;
SELECT kategoria, COUNT(*) AS biletov_count
FROM public.voennyi_bilet
GROUP BY kategoria
ORDER BY biletov_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.15: Выбрать военные билеты призывников, родившихся до 2000 года
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.15 ==='; END $$;
SELECT vb.id_bileta, vb.nomer_bileta, vb.kategoria, vb.voinskoe_zvanie, vb.data_vydachi, p.id_prizivnik
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
WHERE p.data_rozhdeniya < '2000-01-01'
ORDER BY p.data_rozhdeniya;

-- select pg_sleep(5);

-- Запрос 2.1.16: Топ-5 призывных мероприятий по дате проведения
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.16 ==='; END $$;
SELECT id_meropriyatiya, tip_meropriyatiya, data_provedeniya, mesto_provedeniya, fio_comissara
FROM public.prizivnoe_meropriyatie
ORDER BY data_provedeniya DESC
LIMIT 5;

-- select pg_sleep(5);

-- Запрос 2.1.17: Количество и типы призывных мероприятий по каждому комиссару
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.17 ==='; END $$;
SELECT
    c.fio AS comissar_name,
    COUNT(pm.id_meropriyatiya) AS meropriyatiya_count,
    STRING_AGG(DISTINCT pm.tip_meropriyatiya, ', ' ORDER BY pm.tip_meropriyatiya) AS tipy_meropriyatii
FROM public.comissar c
LEFT JOIN public.prizivnoe_meropriyatie pm ON pm.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio
ORDER BY meropriyatiya_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.18: Показать призывные мероприятия и связанных призывников
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.18 ==='; END $$;
SELECT
    pm.id_meropriyatiya,
    pm.tip_meropriyatiya,
    pm.data_provedeniya,
    pm.mesto_provedeniya,
    p.fio AS prizivnik_name
FROM public.prizivnoe_meropriyatie pm
JOIN public.prizivnik_meropriyatie pmm ON pmm.id_meropriyatiya = pm.id_meropriyatiya
JOIN public.prizivnik p ON p.id_prizivnik = pmm.id_prizivnik
ORDER BY pm.id_meropriyatiya;

-- select pg_sleep(5);

-- Запрос 2.1.19: Вывести медицинские освидетельствования с именем призывника и категорией годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.19 ==='; END $$;
SELECT
    mo.id_osvidetelstvovania,
    mo.data_provedeniya,
    mo.fio_vracha,
    mo.zaklyuchenie,
    p.fio AS prizivnik_name,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.med_osvidetelstvovanie mo
LEFT JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = mo.id_kategorii
ORDER BY mo.data_provedeniya DESC;

-- select pg_sleep(5);

-- Запрос 2.1.20: Посчитать количество медицинских освидетельствований по каждому врачу
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.20 ==='; END $$;
SELECT fio_vracha, COUNT(*) AS osvidetelstvovaniya_count
FROM public.med_osvidetelstvovanie
GROUP BY fio_vracha
ORDER BY osvidetelstvovaniya_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.21: Показать призывников и их категории годности – и отобрать те, где категория А
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.21 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE vb.kategoria = 'А' OR kg.nazvanie_kategorii = 'А'
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.22: Показать пары призывник-категория годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.22 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    vb.kategoria,
    kg.id_kategorii AS kategoria_id,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.23: Сгруппировать и получить список категорий годности для каждого призывника
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.23 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    COALESCE(STRING_AGG(CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')), ', ' ORDER BY kg.id_kategorii), 'Нет категории') AS categories_list,
    COUNT(vb.id_bileta) AS categories_count
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
GROUP BY p.id_prizivnik, p.fio
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.24: Посчитать, сколько призывников относится к каждой категории годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.24 ==='; END $$;
SELECT
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY prizivniki_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.25: Суммарное количество призывников по каждой категории годности
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.25 ==='; END $$;
SELECT
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS total_prizivniki,
    COUNT(vb.id_bileta) AS biletov_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY total_prizivniki DESC;

-- select pg_sleep(5);

-- Запрос 2.1.26: Показать категории годности, к которым не привязаны призывники
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.26 ==='; END $$;
SELECT 
    kg.id_kategorii, 
    kg.nazvanie_kategorii, 
    kg.index_kategorii, 
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
WHERE vb.id_bileta IS NULL
ORDER BY kg.id_kategorii;

-- select pg_sleep(5);

-- Запрос 2.1.27: Вывести призывников, у которых категория годности с индексом больше 1
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.27 ==='; END $$;
SELECT
    p.id_prizivnik,
    p.fio,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE kg.index_kategorii IS NOT NULL AND kg.index_kategorii > 1
ORDER BY p.fio;

-- select pg_sleep(5);

-- Запрос 2.1.28: Вывести список категорий годности с читаемым описанием
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.28 ==='; END $$;
SELECT
    kg.id_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii AS description,
    kg.osnovanie_dlya_kategorii AS basis
FROM public.kategoria_godnosti kg
ORDER BY kg.nazvanie_kategorii, COALESCE(kg.index_kategorii, 0);

-- select pg_sleep(5);

-- Запрос 2.1.29: Вывести список категорий годности, которые используются в системе
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.29 ==='; END $$;
SELECT
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii,
    COUNT(vb.id_bileta) AS usage_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii, kg.opisanie_ogranichenii
HAVING COUNT(vb.id_bileta) > 0
ORDER BY usage_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.30: Статистика по категориям годности (сколько призывников в каждой категории)
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.30 ==='; END $$;
SELECT 
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count,
    ROUND(COUNT(DISTINCT p.id_prizivnik) * 100.0 / NULLIF((SELECT COUNT(*) FROM public.voennyi_bilet), 0), 2) AS percentage
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY prizivniki_count DESC;

-- select pg_sleep(5);

-- Запрос 2.1.31: Объединить ФИО всех участников системы (призывников и комиссаров), без дублей
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.31 ==='; END $$;
SELECT fio AS participant_name
FROM public.prizivnik
UNION
SELECT fio AS participant_name
FROM public.comissar
ORDER BY participant_name;

-- select pg_sleep(5);

-- Запрос 2.1.32: Вывести призывников, у которых нет военного билета
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.32 ==='; END $$;
SELECT fio
FROM public.prizivnik
EXCEPT
SELECT p.fio
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY fio;

-- select pg_sleep(5);

-- Запрос 2.1.33: Вывести города, которые встречаются и среди адресов призывников, и среди мест проведения мероприятий
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.33 ==='; END $$;
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.prizivnik
WHERE adres_prozhivaniya IS NOT NULL
INTERSECT
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(mesto_provedeniya, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.prizivnoe_meropriyatie
WHERE mesto_provedeniya IS NOT NULL
ORDER BY city;

-- select pg_sleep(5);

-- Запрос 2.1.34: Вывести категории годности, которые используются в военных билетах и присутствуют в медицинских освидетельствованиях
DO $$ BEGIN RAISE NOTICE '=== Выполняется запрос 2.1.34 ==='; END $$;
WITH common_ids AS (
    SELECT id_kategorii
    FROM public.voennyi_bilet
    WHERE id_kategorii IS NOT NULL
    
    INTERSECT
    
    SELECT id_kategorii
    FROM public.med_osvidetelstvovanie
    WHERE id_kategorii IS NOT NULL
)
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
JOIN common_ids c ON kg.id_kategorii = c.id_kategorii
ORDER BY kg.id_kategorii;

DO $$ BEGIN RAISE NOTICE '=== Все запросы выполнены успешно! ==='; END $$;
