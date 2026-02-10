-- 6.13: Показать военные билеты вместе с ФИО призывника и возрастом призывника
SELECT
    vb.ticket_id,
    vb.ticket_number,
    vb.military_rank,
    vb.category,
    p.full_name AS conscript_name,
    EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.birth_date)) AS conscript_age,
    vb.issue_date
FROM public.military_id_cards vb
JOIN public.conscripts p ON p.conscript_id = vb.conscript_id
ORDER BY p.full_name;
