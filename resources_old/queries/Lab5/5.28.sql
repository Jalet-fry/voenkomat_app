-- Запрос 5.28: Вывести список категорий годности с читаемым описанием
SELECT 
    kg.id_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    kg.opisanie_ogranichenii AS description,
    kg.osnovanie_dlya_kategorii AS basis
FROM public.kategoria_godnosti kg
ORDER BY kg.nazvanie_kategorii, kg.index_kategorii;

