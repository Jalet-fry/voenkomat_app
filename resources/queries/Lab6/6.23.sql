-- 6.23: Сгруппировать и получить список категорий годности для каждого призывника
SELECT
    p.conscript_id,
    p.full_name,
    COALESCE(STRING_AGG(CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')), ', ' ORDER BY kg.category_id), 'Нет категории') AS categories_list,
    COUNT(vb.ticket_id) AS categories_count
FROM public.conscripts p
LEFT JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
LEFT JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
GROUP BY p.conscript_id, p.full_name
ORDER BY p.full_name;
