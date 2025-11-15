-- 2.1.10: Вывести комиссаров и количество призывников в каждом
SELECT
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    c.stazh_raboty,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
ORDER BY prizivniki_count DESC;

