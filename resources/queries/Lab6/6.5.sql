-- 6.5: Посчитать количество призывников по городу
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.residence_address, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(*) AS conscripts_count
FROM public.conscripts p
WHERE p.residence_address IS NOT NULL
GROUP BY 1
ORDER BY conscripts_count DESC;
