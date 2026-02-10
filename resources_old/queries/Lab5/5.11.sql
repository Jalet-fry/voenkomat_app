-- Запрос 5.11: Показать комиссаров и скомпонованный список призывников
SELECT 
    c.id_comissar,
    c.fio,
    c.dolzhnost,
    STRING_AGG(p.fio, ', ' ORDER BY p.fio) AS prizivniki_list
FROM public.comissar c
LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar
LEFT JOIN public.prizivnik p ON p.id_prizivnik = pc.id_prizivnik
GROUP BY c.id_comissar, c.fio, c.dolzhnost
ORDER BY c.fio;

