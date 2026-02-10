-- Запрос 5.9: Вывести комиссаров с должностью "Военный комиссар" и отсортировать по стажу
SELECT 
    id_comissar,
    fio,
    dolzhnost,
    stazh_raboty,
    kontaktnyi_telefon
FROM public.comissar
WHERE dolzhnost = 'Военный комиссар'
ORDER BY stazh_raboty DESC;

