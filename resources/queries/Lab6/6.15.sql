-- 2.1.15: Выбрать военные билеты призывников, родившихся до 2000 года
SELECT vb.id_bileta, vb.nomer_bileta, vb.kategoria, vb.voinskoe_zvanie, vb.data_vydachi, p.id_prizivnik
FROM public.voennyi_bilet vb
JOIN public.prizivnik p ON p.id_prizivnik = vb.id_prizivnika
WHERE p.data_rozhdeniya < '2000-01-01'
ORDER BY p.data_rozhdeniya;

