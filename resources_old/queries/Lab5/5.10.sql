-- Запрос 5.10: Список комиссаров с количеством призывников в каждом (включая комиссаров без призывников)
SELECT 
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    COUNT(pc.id_prizivnik) AS prizivniki_count
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY prizivniki_count DESC;

