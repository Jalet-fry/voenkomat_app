-- 6.6: Выбрать призывников, у которых есть военный билет
SELECT
    p.conscript_id,
    p.full_name,
    p.birth_date,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.birth_date)) AS age
FROM public.conscripts p
WHERE EXISTS (
    SELECT 1 FROM public.military_id_cards vb WHERE vb.conscript_id = p.conscript_id
)
ORDER BY p.full_name;
