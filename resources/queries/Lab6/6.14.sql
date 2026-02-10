-- 6.14: Подсчитать количество военных билетов по категориям годности
SELECT category, COUNT(*) AS biletov_count
FROM public.military_id_cards
GROUP BY category
ORDER BY biletov_count DESC;
