-- 6.20: Посчитать количество медицинских освидетельствований по каждому врачу
SELECT doctor_full_name, COUNT(*) AS examinations_count
FROM public.medical_examinations
GROUP BY doctor_full_name
ORDER BY examinations_count DESC;
