-- 2.1.30: Статистика по категориям годности (сколько призывников в каждой категории)
SELECT 
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count,
    ROUND(COUNT(DISTINCT p.id_prizivnik) * 100.0 / NULLIF((SELECT COUNT(*) FROM public.voennyi_bilet), 0), 2) AS percentage
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY prizivniki_count DESC;

