-- 6.11: Показать комиссаров с должностью "Военный комиссар" и отсортировать по стажу
SELECT commissioner_id, full_name, position, years_of_service, phone_number
FROM public.commissioners
WHERE position = 'Военный комиссар'
ORDER BY years_of_service DESC;
