-- 2.1.8: Топ-5 призывников по количеству связанных с ними мероприятий
SELECT
    p.id_prizivnik,
    p.fio,
    COALESCE(COUNT(pm.id_meropriyatiya), 0) AS meropriyatiya_count
FROM public.prizivnik p
LEFT JOIN public.prizivnik_meropriyatie pm 
    ON pm.id_prizivnik = p.id_prizivnik
GROUP BY p.id_prizivnik, p.fio
HAVING COALESCE(COUNT(pm.id_meropriyatiya), 0) > 0
ORDER BY meropriyatiya_count DESC
LIMIT 5;

