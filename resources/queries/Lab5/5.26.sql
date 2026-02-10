-- 5.26: Показать категории годности, к которым не привязаны призывники
SELECT 
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, kg.category_index) AS full_category
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
WHERE vb.ticket_id IS NULL
ORDER BY kg.category_id;
