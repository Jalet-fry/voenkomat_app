-- Запрос 5.5: Посчитать количество призывников по воинским званиям
SELECT 
    vb.voinskoe_zvanie,
    COUNT(p.id_prizivnik) AS count_prizivniki
FROM public.voennyi_bilet vb
LEFT JOIN public.prizivnik p ON p.id_voennogo_bileta = vb.id_bileta
GROUP BY vb.voinskoe_zvanie
ORDER BY count_prizivniki DESC;

