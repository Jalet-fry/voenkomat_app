-- 6.34: Вывести категории годности, которые используются в военных билетах и присутствуют в медицинских освидетельствованиях
WITH common_ids AS (
    SELECT category_id
    FROM public.military_id_cards
    WHERE category_id IS NOT NULL
    
    INTERSECT
    
    SELECT category_id
    FROM public.medical_examinations
    WHERE category_id IS NOT NULL
)
SELECT 
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category
FROM public.fitness_categories kg
JOIN common_ids c ON kg.category_id = c.category_id
ORDER BY kg.category_id;
