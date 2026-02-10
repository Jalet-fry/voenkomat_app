-- 5.7: Для каждого комиссара показать количество связанных призывников
SELECT 
    c.full_name,
    COUNT(pc.conscript_id) AS count
FROM public.commissioners c
LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id
GROUP BY c.commissioner_id, c.full_name
ORDER BY count DESC;
