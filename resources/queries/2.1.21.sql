-- 2.1.21: Показать призывников и их категории годности – и отобрать те, где категория А
SELECT
    p.id_prizivnik,
    p.fio,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE vb.kategoria = 'А' OR kg.nazvanie_kategorii = 'А'
ORDER BY p.fio;

