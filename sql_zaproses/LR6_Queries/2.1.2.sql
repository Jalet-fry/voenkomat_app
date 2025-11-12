-- 2.1.2: Посчитать число призывников по городам и показать только города с 1 призывником
SELECT
    TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city,
    COUNT(p.id_prizivnik) AS prizivniki_count
FROM public.prizivnik p
WHERE p.adres_prozhivaniya IS NOT NULL
GROUP BY TRIM(REPLACE(REPLACE(SPLIT_PART(p.adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', ''))
HAVING COUNT(p.id_prizivnik) = 1
ORDER BY prizivniki_count DESC;

