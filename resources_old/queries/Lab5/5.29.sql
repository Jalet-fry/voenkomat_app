-- Запрос 5.29: Вывести список категорий годности, которые используются в системе
SELECT 
    kg.nazvanie_kategorii,
    COUNT(vb.id_bileta) AS total_prizivniki,
    COUNT(DISTINCT vb.id_prizivnika) AS unique_prizivniki
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
GROUP BY kg.nazvanie_kategorii
ORDER BY total_prizivniki DESC;

