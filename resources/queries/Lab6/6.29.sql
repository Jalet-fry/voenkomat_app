-- 6.29: Вывести список категорий годности, которые используются в системе
SELECT
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category,
    kg.restriction_description,
    COUNT(vb.ticket_id) AS usage_count
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
GROUP BY kg.category_id, kg.category_name, kg.category_index, kg.restriction_description
HAVING COUNT(vb.ticket_id) > 0
ORDER BY usage_count DESC;
