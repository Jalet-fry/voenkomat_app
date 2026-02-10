-- 6.24: Посчитать, сколько призывников относится к каждой категории годности
SELECT
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category,
    COUNT(DISTINCT p.conscript_id) AS conscripts_count
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
LEFT JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
GROUP BY kg.category_id, kg.category_name, kg.category_index
ORDER BY conscripts_count DESC;
