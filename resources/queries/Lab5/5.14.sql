-- 5.14: Вывести военные билеты, выданные после 2023-01-01
SELECT ticket_number, military_rank, category, issue_date
FROM public.military_id_cards
WHERE issue_date > '2023-01-01'
ORDER BY issue_date;
