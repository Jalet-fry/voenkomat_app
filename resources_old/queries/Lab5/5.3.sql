-- Запрос 5.3: Вывести призывников старше 20 лет с форматированием ФИО и номера паспорта
SELECT 
    id_prizivnik,
    fio,
    nomer_pasporta,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) AS age,
    CONCAT(fio, ' — паспорт: ', nomer_pasporta) AS info
FROM public.prizivnik
WHERE EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) > 20
ORDER BY age DESC, fio;

