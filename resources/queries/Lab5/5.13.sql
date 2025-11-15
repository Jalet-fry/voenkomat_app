-- Запрос 5.13: Показать военные билеты вместе с ФИО призывника и возрастом
SELECT 
    vb.id_bileta,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    p.fio AS prizivnik_name,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS prizivnik_age
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
ORDER BY p.fio;

