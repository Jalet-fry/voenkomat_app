-- 2.1.33: Вывести города, которые встречаются и среди адресов призывников, и среди мест проведения мероприятий
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(adres_prozhivaniya, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.prizivnik
WHERE adres_prozhivaniya IS NOT NULL
INTERSECT
SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(mesto_provedeniya, ',', 1), 'г.', ''), 'г ', '')) AS city
FROM public.prizivnoe_meropriyatie
WHERE mesto_provedeniya IS NOT NULL
ORDER BY city;

