-- 6.2: Посчитать число призывников по городам и показать только города с 1 призывником
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.residence_address, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(p.conscript_id) AS conscripts_count
FROM public.conscripts p
WHERE p.residence_address IS NOT NULL
GROUP BY 1
HAVING COUNT(p.conscript_id) = 1
ORDER BY conscripts_count DESC;
