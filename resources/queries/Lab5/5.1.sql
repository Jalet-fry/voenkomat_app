-- 5.1: Призывники по возрасту
SELECT full_name, birth_date, EXTRACT(YEAR FROM AGE(CURRENT_DATE, birth_date)) as age FROM public.conscripts ORDER BY age DESC;
