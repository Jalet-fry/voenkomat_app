-- 6.9: Найти призывников, не имеющих ни одного медицинского освидетельствования
SELECT p.conscript_id, p.full_name, p.birth_date
FROM public.conscripts p
WHERE NOT EXISTS (
    SELECT 1 FROM public.medical_examinations mo WHERE mo.conscript_id = p.conscript_id
)
ORDER BY p.full_name;
