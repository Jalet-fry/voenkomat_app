-- 2.1.34: Вывести категории годности, которые используются в военных билетах и присутствуют в медицинских освидетельствованиях
WITH common_ids AS (
    SELECT id_kategorii
    FROM public.voennyi_bilet
    WHERE id_kategorii IS NOT NULL
    
    INTERSECT
    
    SELECT id_kategorii
    FROM public.med_osvidetelstvovanie
    WHERE id_kategorii IS NOT NULL
)
SELECT 
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.kategoria_godnosti kg
JOIN common_ids c ON kg.id_kategorii = c.id_kategorii
ORDER BY kg.id_kategorii;

