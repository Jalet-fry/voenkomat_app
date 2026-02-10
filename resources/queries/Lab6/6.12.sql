-- 6.12: Найти комиссара(ов) с максимальным стажем работы
SELECT commissioner_id, full_name, position, years_of_service
FROM public.commissioners
WHERE years_of_service = (SELECT MAX(years_of_service) FROM public.commissioners);
