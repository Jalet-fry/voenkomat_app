-- 5.30: Статистика по категориям годности (процентное соотношение)
SELECT 
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, kg.category_index) AS full_category,
    COUNT(DISTINCT p.conscript_id) AS conscripts_count,
    ROUND(COUNT(DISTINCT p.conscript_id) * 100.0 / NULLIF((SELECT COUNT(*) FROM public.conscripts), 0), 2) AS percentage
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
LEFT JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
GROUP BY kg.category_id, kg.category_name, kg.category_index
ORDER BY conscripts_count DESC;
