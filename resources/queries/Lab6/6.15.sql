-- 6.15: Выбрать военные билеты призывников, родившихся до 2000 года
SELECT vb.ticket_id, vb.ticket_number, vb.category, vb.military_rank, vb.issue_date, p.conscript_id
FROM public.military_id_cards vb
JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
WHERE p.birth_date < '2000-01-01'
ORDER BY p.birth_date;
