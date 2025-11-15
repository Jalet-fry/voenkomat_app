-- 2.1.16: Топ-5 призывных мероприятий по дате проведения
SELECT id_meropriyatiya, tip_meropriyatiya, data_provedeniya, mesto_provedeniya, fio_comissara
FROM public.prizivnoe_meropriyatie
ORDER BY data_provedeniya DESC
LIMIT 5;

