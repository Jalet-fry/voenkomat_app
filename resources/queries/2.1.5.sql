-- 2.1.5: Посчитать количество призывников по городу
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(*) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
ORDER BY prizivniki_count DESC;

