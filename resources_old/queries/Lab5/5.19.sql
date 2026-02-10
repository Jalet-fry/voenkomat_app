-- Запрос 5.19: Вывести медицинские освидетельствования с именем призывника и категорией годности
SELECT 
    mo.id_osvidetelstvovania,
    mo.data_provedeniya,
    mo.fio_vracha,
    mo.zaklyuchenie,
    p.fio AS prizivnik_name,
    kg.nazvanie_kategorii,
    kg.opisanie_ogranichenii
FROM public.med_osvidetelstvovanie mo
JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = mo.id_kategorii
ORDER BY mo.data_provedeniya DESC;

