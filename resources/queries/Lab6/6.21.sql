-- 6.21: Показать призывников и их категории годности – и отобрать те, где категория А
SELECT
    p.conscript_id,
    p.full_name,
    vb.category,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category
FROM public.conscripts p
JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
WHERE vb.category = 'А' OR kg.category_name = 'А'
ORDER BY p.full_name;
