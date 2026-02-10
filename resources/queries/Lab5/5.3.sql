-- 5.3: Призывники старше 20 лет
SELECT full_name, passport_number, EXTRACT(YEAR FROM AGE(CURRENT_DATE, birth_date)) as age
FROM public.conscripts WHERE EXTRACT(YEAR FROM AGE(CURRENT_DATE, birth_date)) > 20;
