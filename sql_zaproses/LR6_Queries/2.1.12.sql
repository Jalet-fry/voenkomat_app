-- 2.1.12: Найти комиссара(ов) с максимальным стажем работы
SELECT c.id_comissar, c.fio, c.dolzhnost, c.stazh_raboty
FROM public.comissar c
WHERE c.stazh_raboty = (SELECT MAX(stazh_raboty) FROM public.comissar);

