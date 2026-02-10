-- Запрос 5.30: Статистика по категориям годности (сколько призывников в каждой категории)
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    COUNT(DISTINCT p.id_prizivnik) AS prizivniki_count,
    ROUND(COUNT(DISTINCT p.id_prizivnik) * 100.0 / NULLIF((SELECT COUNT(*) FROM public.prizivnik), 0), 2) AS percentage
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY prizivniki_count DESC;

