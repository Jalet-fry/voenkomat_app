-- 6.3: Для каждого призывника показать, сколько медицинских освидетельствований связано с ним
SELECT
    p.conscript_id,
    p.full_name,
    (SELECT COUNT(*) FROM public.medical_examinations mo WHERE mo.conscript_id = p.conscript_id) AS examinations_count
FROM public.conscripts p
ORDER BY examinations_count DESC, p.full_name;
