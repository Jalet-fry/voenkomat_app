-- 2.1.6: Выбрать призывников, у которых есть военный билет
SELECT
    p.id_prizivnik,
    p.fio,
    p.data_rozhdeniya,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age
FROM public.prizivnik p
WHERE EXISTS (
    SELECT 1 FROM public.voennyi_bilet vb WHERE vb.id_prizivnika = p.id_prizivnik
)
ORDER BY p.fio;

