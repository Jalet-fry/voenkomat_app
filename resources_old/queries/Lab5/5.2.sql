-- Запрос 5.2: Посчитать количество призывников по городам проживания
SELECT 
    TRIM(SPLIT_PART(adres_prozhivaniya, ',', 1)) AS city,
    COUNT(*) AS prizivniki_count
FROM public.prizivnik
GROUP BY city
ORDER BY prizivniki_count DESC;

