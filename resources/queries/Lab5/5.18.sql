-- Запрос 5.18: Показать призывные мероприятия и связанных призывников
SELECT 
    pm.id_meropriyatiya,
    pm.tip_meropriyatiya,
    pm.data_provedeniya,
    pm.mesto_provedeniya,
    p.fio AS prizivnik_name
FROM public.prizivnoe_meropriyatie pm
JOIN public.prizivnik_meropriyatie pmm ON pmm.id_meropriyatiya = pm.id_meropriyatiya
JOIN public.prizivnik p ON p.id_prizivnik = pmm.id_prizivnik
ORDER BY pm.id_meropriyatiya;

