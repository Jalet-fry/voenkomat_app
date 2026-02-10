-- 6.27: Вывести призывников, у которых категория годности с индексом больше 1
SELECT
    p.conscript_id,
    p.full_name,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category
FROM public.conscripts p
JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
WHERE kg.category_index IS NOT NULL AND kg.category_index > 1
ORDER BY p.full_name;
