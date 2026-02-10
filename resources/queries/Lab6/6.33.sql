-- 6.33: Вывести города, которые встречаются и среди адресов призывников, и среди мест проведения мероприятий
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(residence_address, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.conscripts
WHERE residence_address IS NOT NULL
INTERSECT
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(event_location, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.callup_events
WHERE event_location IS NOT NULL
ORDER BY city;
