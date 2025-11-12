-- 2.1.19: Вывести медицинские освидетельствования с именем призывника и категорией годности
SELECT
    mo.id_osvidetelstvovania,
    mo.data_provedeniya,
    mo.fio_vracha,
    mo.zaklyuchenie,
    p.fio AS prizivnik_name,
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, COALESCE(kg.index_kategorii::text, '')) AS full_category
FROM public.med_osvidetelstvovanie mo
LEFT JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika
LEFT JOIN public.kategoria_godnosti kg ON kg.id_kategorii = mo.id_kategorii
ORDER BY mo.data_provedeniya DESC;

