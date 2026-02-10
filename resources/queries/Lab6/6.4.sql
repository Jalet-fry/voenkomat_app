-- 6.4: Показать призывников с их военными билетами (номер билета, воинское звание, категория)
SELECT
    p.conscript_id,
    p.full_name,
    p.birth_date,
    vb.ticket_number,
    vb.military_rank,
    vb.category,
    vb.issue_date
FROM public.conscripts p
LEFT JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id
ORDER BY p.full_name;
