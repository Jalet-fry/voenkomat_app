-- 5.4: Вывести призывников с их военными билетами
SELECT 
    p.full_name,
    vb.ticket_number,
    vb.military_rank
FROM public.conscripts p
LEFT JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id;
