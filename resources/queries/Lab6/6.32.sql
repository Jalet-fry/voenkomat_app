-- 2.1.32: Вывести призывников, у которых нет военного билета
SELECT fio
FROM public.prizivnik
EXCEPT
SELECT p.fio
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY fio;

