-- Запрос 5.21: Для каждой категории годности вывести количество, средний возраст призывников
SELECT 
    kg.nazvanie_kategorii,
    kg.index_kategorii,
    CONCAT(kg.nazvanie_kategorii, kg.index_kategorii) AS full_category,
    COUNT(p.id_prizivnik) AS prizivniki_count,
    ROUND(AVG(EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)))::numeric, 2) AS avg_age
FROM public.kategoria_godnosti kg
LEFT JOIN public.voennyi_bilet vb ON vb.id_kategorii = kg.id_kategorii
LEFT JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
GROUP BY kg.id_kategorii, kg.nazvanie_kategorii, kg.index_kategorii
ORDER BY kg.nazvanie_kategorii, kg.index_kategorii;

