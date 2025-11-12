-- 2.1.29: Вывести список категорий годности, которые используются в системе
SELECT
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii,
    COUNT(vb.id_bileta) AS usage_count
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii, kg.opisanie_ogranichenii
HAVING COUNT(vb.id_bileta) > 0
ORDER BY usage_count DESC;

