-- Запрос 5.6: Вывести всех призывников с категорией годности А
SELECT 
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.kategoria,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
WHERE vb.kategoria = 'А'
ORDER BY p.fio;

