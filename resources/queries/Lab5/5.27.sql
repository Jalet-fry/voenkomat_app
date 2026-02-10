-- 5.27: Показать военные билеты вместе с полной информацией о категории годности
SELECT 
    vb.ticket_id,
    vb.ticket_number,
    vb.military_rank,
    vb.category,
    vb.issue_date,
    kg.category_id,
    kg.category_name,
    kg.category_index,
    CONCAT(kg.category_name, kg.category_index) AS full_category,
    kg.restriction_description,
    kg.category_basis
FROM public.military_id_cards vb
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
ORDER BY vb.issue_date DESC;
