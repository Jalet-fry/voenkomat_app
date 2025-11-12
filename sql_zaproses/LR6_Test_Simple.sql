-- Простые тестовые запросы для проверки данных
-- Выполняйте по одному запросу, чтобы видеть результаты

-- 1. Проверка данных в таблицах
SELECT 'Призывники' AS table_name, COUNT(*) AS count FROM public.prizivnik
UNION ALL
SELECT 'Военные билеты', COUNT(*) FROM public.voennyi_bilet
UNION ALL
SELECT 'Категории годности', COUNT(*) FROM public.kategoria_godnosti;

-- 2. Простой SELECT без JOIN
SELECT * FROM public.prizivnik LIMIT 5;

-- 3. SELECT с одним JOIN
SELECT 
    p.id_prizivnik,
    p.fio,
    vb.nomer_bileta
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
LIMIT 5;

-- 4. SELECT с двумя JOIN
SELECT 
    p.id_prizivnik,
    p.fio,
    kg.nazvanie_kategorii
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
LIMIT 5;

