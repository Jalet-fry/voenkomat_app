-- 5.2: Количество по городам
SELECT TRIM(SPLIT_PART(residence_address, ',', 1)) as city, COUNT(*) FROM public.conscripts GROUP BY 1 ORDER BY 2 DESC;
