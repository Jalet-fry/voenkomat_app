-- 6.25: Суммарное количество призывников по каждой категории годности
SELECT
    kg.category_name,
    COUNT(DISTINCT p.conscript_id) AS total_conscripts,
    COUNT(vb.ticket_id) AS biletov_count
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
LEFT JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
GROUP BY kg.category_name
ORDER BY total_conscripts DESC;
