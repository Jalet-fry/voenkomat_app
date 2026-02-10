-- Запрос 5.8: Топ-5 комиссаров по стажу работы
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE stazh_raboty IS NOT NULL
ORDER BY stazh_raboty DESC
LIMIT 5;

