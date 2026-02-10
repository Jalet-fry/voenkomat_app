-- 6.28: Вывести список категорий годности с читаемым описанием
SELECT
    kg.category_id,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category,
    kg.restriction_description AS description,
    kg.category_basis AS basis
FROM public.fitness_categories kg
ORDER BY kg.category_name, COALESCE(kg.category_index, 0);
