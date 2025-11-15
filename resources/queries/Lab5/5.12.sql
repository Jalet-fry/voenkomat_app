-- Запрос 5.12: Вывести комиссаров со стажем работы более 10 лет, упорядочить по стажу
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE stazh_raboty > 10
ORDER BY stazh_raboty DESC;

