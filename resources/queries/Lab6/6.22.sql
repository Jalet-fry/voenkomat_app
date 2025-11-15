-- 2.1.22: Показать пары призывник-категория годности
SELECT
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    vb.kategoria,
    kg.id_kategorii AS kategoria_id,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category,
    kg.opisanie_ogranichenii
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY p.fio;

