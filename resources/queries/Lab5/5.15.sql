-- 5.15: Подсчитать количество военных билетов по категориям
SELECT category, COUNT(*) AS count
FROM public.military_id_cards
GROUP BY category
ORDER BY count DESC;
