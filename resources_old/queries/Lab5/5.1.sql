-- Запрос 5.1: Вывести призывников с ФИО, датой рождения и возрастом, отсортировать по возрасту по убыванию
SELECT 
    id_prizivnik,
    fio,
    data_rozhdeniya,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) AS age
FROM public.prizivnik
ORDER BY age DESC;

