-- 5.21: Для каждой категории годности вывести количество и средний возраст призывников
SELECT 
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, kg.category_index) AS full_category,
    COUNT(p.conscript_id) AS prizivniki_count,
    ROUND(AVG(EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.birth_date)))::numeric, 2) AS avg_age
FROM public.fitness_categories kg
LEFT JOIN public.military_id_cards vb ON vb.category_id = kg.category_id
LEFT JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
GROUP BY kg.category_id, kg.category_name, kg.category_index
ORDER BY kg.category_name, kg.category_index;
