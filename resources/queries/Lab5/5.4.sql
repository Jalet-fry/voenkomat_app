-- Запрос 5.4: Вывести призывников с их военными билетами
SELECT 
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
ORDER BY p.fio;

