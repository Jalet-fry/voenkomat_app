-- 5.18: Показать призывные мероприятия и связанных призывников
SELECT 
    ce.event_id,
    ce.event_type,
    ce.event_datetime,
    ce.event_location,
    p.full_name AS conscript_name
FROM public.callup_events ce
JOIN public.conscripts_events c_e ON c_e.event_id = ce.event_id
JOIN public.conscripts p ON p.conscript_id = c_e.conscript_id
ORDER BY ce.event_id;
