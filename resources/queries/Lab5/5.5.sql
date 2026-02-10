-- 5.5: Посчитать количество призывников по воинским званиям
SELECT military_rank, COUNT(*) as count
FROM public.military_id_cards
GROUP BY military_rank
ORDER BY count DESC;
