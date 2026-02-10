-- 5.22: Показать пары призывник-категория годности
SELECT 
    p.full_name AS conscript_name,
    vb.category AS ticket_category,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, kg.category_index) AS full_category,
    kg.restriction_description
FROM public.conscripts p
JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
ORDER BY p.full_name;
