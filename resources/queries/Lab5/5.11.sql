-- Запрос 5.11: Показать комиссаров и скомпонованный список призывников
SELECT 
    c.commissioner_id,
    c.full_name,
    STRING_AGG(p.full_name, ', ' ORDER BY p.full_name) AS conscripts_list
FROM public.commissioners c
LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id
LEFT JOIN public.conscripts p ON p.conscript_id = pc.conscript_id
GROUP BY c.commissioner_id, c.full_name
ORDER BY c.full_name;
