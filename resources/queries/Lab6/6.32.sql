-- 6.32: Вывести призывников, у которых нет военного билета
SELECT full_name
FROM public.conscripts
EXCEPT
SELECT p.full_name
FROM public.military_id_cards vb
JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
ORDER BY full_name;
