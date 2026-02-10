-- 5.28: Вывести список категорий годности с читаемым описанием
SELECT 
    category_id,
    CONCAT(category_name, category_index) AS full_category,
    restriction_description AS description,
    category_basis AS basis
FROM public.fitness_categories
ORDER BY category_name, category_index;
