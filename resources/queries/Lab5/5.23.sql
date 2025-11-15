-- Запрос 5.23: Сгруппировать и получить список категорий годности для каждого призывника
SELECT 
    p.id_prizivnik,
    p.fio AS prizivnik_name,
    STRING_AGG(CONCAT(kg.nazvanie_kategorii, kg.index_kategorii), ', ' ORDER BY kg.id_kategorii) AS categories_list,
    COUNT(vb.id_bileta) AS categories_count
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
GROUP BY p.id_prizivnik, p.fio
ORDER BY p.fio;

