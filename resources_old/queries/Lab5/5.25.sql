-- Запрос 5.25: Суммарное количество призывников по каждой категории годности
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    COUNT(DISTINCT p.id_prizivnik) AS total_prizivniki
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY total_prizivniki DESC;

