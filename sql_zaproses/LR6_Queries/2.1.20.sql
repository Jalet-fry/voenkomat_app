-- 2.1.20: Посчитать количество медицинских освидетельствований по каждому врачу
SELECT fio_vracha, COUNT(*) AS osvidetelstvovaniya_count
FROM public.med_osvidetelstvovanie
GROUP BY fio_vracha
ORDER BY osvidetelstvovaniya_count DESC;

