-- 5.29: Вывести список категорий годности, которые используются в системе
SELECT 
    kg.category_name,
    COUNT(vb.ticket_id) AS total_conscripts,
    COUNT(DISTINCT vb.conscript_id) AS unique_conscripts
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
GROUP BY kg.category_name
ORDER BY total_conscripts DESC;
