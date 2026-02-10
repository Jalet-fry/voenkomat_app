-- 6.17: Количество и типы призывных мероприятий по каждому комиссару
SELECT
    c.full_name AS commissioner_name,
    COUNT(ce.event_id) AS events_count,
    STRING_AGG(DISTINCT ce.event_type, ', ' ORDER BY ce.event_type) AS event_types
FROM public.commissioners c
LEFT JOIN public.callup_events ce ON ce.commissioner_id = c.commissioner_id
GROUP BY c.commissioner_id, c.full_name
ORDER BY events_count DESC;
