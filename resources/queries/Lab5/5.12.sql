-- 5.12: Вывести комиссаров со стажем работы более 10 лет
SELECT full_name, position, years_of_service, phone_number
FROM public.commissioners
WHERE years_of_service > 10
ORDER BY years_of_service DESC;
