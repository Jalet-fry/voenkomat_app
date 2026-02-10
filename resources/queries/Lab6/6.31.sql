-- 6.31: Объединить ФИО всех участников системы (призывников и комиссаров), без дублей
SELECT full_name AS participant_name
FROM public.conscripts
UNION
SELECT full_name AS participant_name
FROM public.commissioners
ORDER BY participant_name
LIMIT 10;
