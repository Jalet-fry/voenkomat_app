-- 2.1.31: Объединить ФИО всех участников системы (призывников и комиссаров), без дублей
SELECT fio AS participant_name
FROM public.prizivnik
UNION
SELECT fio AS participant_name
FROM public.comissar
ORDER BY participant_name
LIMIT 10;

