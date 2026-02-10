-- 5.16: Топ-5 призывных мероприятий по дате проведения
SELECT event_id, event_type, event_datetime, event_location, commissioner_full_name
FROM public.callup_events
ORDER BY event_datetime DESC
LIMIT 5;
