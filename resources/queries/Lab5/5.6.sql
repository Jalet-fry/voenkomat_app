-- 5.6: Все призывники с категорией годности А
SELECT 
    p.full_name,
    kg.category_name
FROM public.conscripts p
JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
JOIN public.fitness_categories kg ON kg.category_id = vb.category_id
WHERE kg.category_name = 'А';
