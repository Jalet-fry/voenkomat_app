-- Запрос 5.17: Количество и типы призывных мероприятий по каждому комиссару
SELECT 
    c.fio AS comissar_name,
    COUNT(pm.id_meropriyatiya) AS meropriyatiya_count,
    STRING_AGG(DISTINCT pm.tip_meropriyatiya, ', ') AS tipy_meropriyatii
FROM public.comissar c
LEFT JOIN public.prizivnoe_meropriyatie pm ON pm.id_comissar = c.id_comissar
GROUP BY c.id_comissar, c.fio
ORDER BY meropriyatiya_count DESC;

