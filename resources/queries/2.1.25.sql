-- 2.1.25: Суммарное количество призывников по каждой категории годности
SELECT
    kg.nazvanie_kategorii,
    COUNT(DISTINCT p.id_prizivnik) AS total_prizivniki,
    COUNT(vb.id_bileta) AS biletov_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.nazvanie_kategorii
ORDER BY total_prizivniki DESC;

