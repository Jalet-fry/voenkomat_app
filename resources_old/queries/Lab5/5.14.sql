-- Запрос 5.14: Вывести военные билеты, выданные после 2023-01-01, с сортировкой по дате выдачи
SELECT 
    id_bileta,
    nomer_bileta,
    voinskoe_zvanie,
    kategoria,
    data_vydachi
FROM public.voennyi_bilet
WHERE data_vydachi > '2023-01-01'
ORDER BY data_vydachi;

