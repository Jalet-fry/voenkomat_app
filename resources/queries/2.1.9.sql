-- 2.1.9: Найти призывников, не имеющих ни одного медицинского освидетельствования
SELECT p.id_prizivnik, p.fio, p.data_rozhdeniya
FROM public.prizivnik p
WHERE NOT EXISTS (
    SELECT 1 FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;

