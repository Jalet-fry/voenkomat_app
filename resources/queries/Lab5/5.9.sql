-- 5.9: Вывести комиссаров с должностью "Военный комиссар"
SELECT commissioner_id, full_name, years_of_service, phone_number
FROM public.commissioners
WHERE position = 'Военный комиссар'
ORDER BY years_of_service DESC;
