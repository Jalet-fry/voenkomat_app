-- 6.19: Вывести медицинские освидетельствования с именем призывника и категорией годности
SELECT
    mo.certification_id,
    mo.examination_date,
    mo.doctor_full_name,
    mo.conclusion,
    p.full_name AS conscript_name,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, COALESCE(kg.category_index::text, '')) AS full_category
FROM public.medical_examinations mo
LEFT JOIN public.conscripts p ON p.conscript_id = mo.conscript_id
LEFT JOIN public.fitness_categories kg ON kg.category_id = mo.category_id
ORDER BY mo.examination_date DESC;
