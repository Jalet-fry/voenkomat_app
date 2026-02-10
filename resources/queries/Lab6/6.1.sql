-- 6.1: Вывести призывников с названием категории годности и возрастом
SELECT
    p.conscript_id,
    p.full_name,
    p.passport_number,
    kg.category_name AS kategoria_godnosti,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.birth_date)) AS age,
    p.birth_date
FROM public.conscripts p
JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
ORDER BY age DESC, p.full_name;
