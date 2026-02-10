-- Запрос 2.1.1: Вывести призывников с названием категории годности и возрастом (в годах), отсортировать по возрасту (убыв.)
SELECT
    p.id_prizivnik,
    p.fio,
    p.nomer_pasporta,
    kg.nazvanie_kategorii AS kategoria_godnosti,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.data_rozhdeniya)) AS age,
    p.data_rozhdeniya
FROM public.prizivnik p
JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik
JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii
ORDER BY age DESC, p.fio;

