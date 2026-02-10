-- Запрос 5.15: Подсчитать количество военных билетов по категориям
SELECT 
    kategoria,
    COUNT(*) AS biletov_count
FROM public.voennyi_bilet
GROUP BY kategoria
ORDER BY biletov_count DESC;

