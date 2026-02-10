-- 6.7: Для каждого комиссара показать количество связанных призывников
SELECT
    c.commissioner_id,
    c.full_name,
    c.position,
    COUNT(pc.conscript_id) AS conscripts_count
FROM public.commissioners c
LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id
GROUP BY c.commissioner_id, c.full_name, c.position
ORDER BY conscripts_count DESC, c.full_name;
