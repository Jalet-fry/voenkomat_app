-- Запрос 5.10: Список комиссаров с количеством призывников в каждом
SELECT 
    c.commissioner_id,
    c.full_name,
    c.position,
    COUNT(pc.conscript_id) AS prizivniki_count
FROM public.commissioners c
LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id
GROUP BY c.commissioner_id, c.full_name, c.position
ORDER BY prizivniki_count DESC;
