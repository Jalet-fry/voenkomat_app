-- 6.8: Топ-5 призывников по количеству связанных с ними мероприятий
SELECT
    p.conscript_id,
    p.full_name,
    COALESCE(COUNT(ce.event_id), 0) AS events_count
FROM public.conscripts p
LEFT JOIN public.conscripts_events ce
    ON ce.conscript_id = p.conscript_id
GROUP BY p.conscript_id, p.full_name
HAVING COALESCE(COUNT(ce.event_id), 0) > 0
ORDER BY events_count DESC
LIMIT 5;
