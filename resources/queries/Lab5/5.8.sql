-- 5.8: Топ-5 комиссаров по стажу работы
SELECT full_name, position, years_of_service, phone_number
FROM public.commissioners
ORDER BY years_of_service DESC NULLS LAST
LIMIT 5;
