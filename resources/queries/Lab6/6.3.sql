-- 2.1.3: Для каждого призывника показать, сколько медицинских освидетельствований связано с ним
SELECT
    p.id_prizivnik,
    p.fio,
    (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik) AS med_osvidetelstvovaniya_count
FROM public.prizivnik p
ORDER BY med_osvidetelstvovaniya_count DESC, p.fio;

