-- 2.1.26: Показать категории годности, к которым не привязаны призывники
SELECT 
    kg.id_kategorii, 
    kg.nazvanie_kategorii, 
    kg.index_kategorii, 
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
WHERE vb.id_bileta IS NULL
ORDER BY kg.id_kategorii;

