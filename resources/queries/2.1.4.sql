-- 2.1.4: Показать призывников с их военными билетами (номер билета, воинское звание, категория)
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    vb.nomer_bileta,
    vb.voinskoe_zvanie,
    vb.kategoria,
    vb.data_vydachi
FROM public.prizivnik p
LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
ORDER BY p.fio;

