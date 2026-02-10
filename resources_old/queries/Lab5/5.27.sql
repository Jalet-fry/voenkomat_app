-- Запрос 5.27: Показать военные билеты вместе с полной информацией о категории годности
SELECT 
    vb.id_bileta,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    vb.data_vydachi,
    kg.id_kategorii,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    kg.opisanie_ogranichenii,
    kg.osnovanie_dlya_kategorii
FROM public.voennyi_bilet vb
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY vb.data_vydachi DESC;

