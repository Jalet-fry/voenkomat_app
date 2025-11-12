-- 2.1.27: Вывести призывников, у которых категория годности с индексом больше 1
SELECT
    p.id_prizivnik,
    p.fio,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE kg.index_kategorii IS NOT NULL AND kg.index_kategorii > 1
ORDER BY p.fio;

