#include "EmbeddedQueries.h"
#include "QueriesWindow.h"
#include "DbConstants.h"
#include <QDebug>

QList<QueryInfo> getEmbeddedQueries()
{
    QList<QueryInfo> queries;
    QueryInfo query;

    // --- ЛАБОРАТОРНАЯ №5 (Выборка и агрегация) ---

    query.number = "5.1";
    query.description = "Призывники по возрасту (убывание)";
    query.type = "Lab5";
    query.sqlText = "SELECT full_name, birth_date, EXTRACT(YEAR FROM AGE(CURRENT_DATE, birth_date)) as age FROM public.conscripts ORDER BY age DESC;";
    queries.append(query);

    query.number = "5.4";
    query.description = "Призывники с их военными билетами";
    query.type = "Lab5";
    query.sqlText = "SELECT p.full_name, vb.ticket_number, vb.military_rank FROM public.conscripts p LEFT JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id;";
    queries.append(query);

    query.number = "5.7";
    query.description = "Количество призывников у каждого комиссара";
    query.type = "Lab5";
    query.sqlText = "SELECT c.full_name, COUNT(pc.conscript_id) AS count FROM public.commissioners c LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id GROUP BY c.commissioner_id, c.full_name ORDER BY count DESC;";
    queries.append(query);

    query.number = "5.10";
    query.description = "Комиссары и количество их призывников";
    query.type = "Lab5";
    query.sqlText = "SELECT c.full_name, c.position, COUNT(pc.conscript_id) AS prizivniki_count FROM public.commissioners c LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id GROUP BY c.commissioner_id, c.full_name, c.position ORDER BY prizivniki_count DESC;";
    queries.append(query);

    // --- ЛАБОРАТОРНАЯ №6 (Сложные соединения и подзапросы) ---

    query.number = "6.1";
    query.description = "Призывники, категории годности и возраст";
    query.type = "Lab6";
    query.sqlText = "SELECT p.full_name, kg.category_name AS kategoria_godnosti, EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.birth_date)) AS age FROM public.conscripts p JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id JOIN public.fitness_categories kg ON kg.category_id = vb.category_id ORDER BY age DESC, p.full_name;";
    queries.append(query);

    query.number = "6.2";
    query.description = "Города с ровно одним призывником";
    query.type = "Lab6";
    query.sqlText = "SELECT TRIM(REPLACE(REPLACE(SPLIT_PART(p.residence_address, ',', 1), 'г.', ''), 'г ', '')) AS city, COUNT(p.conscript_id) AS conscripts_count FROM public.conscripts p WHERE p.residence_address IS NOT NULL GROUP BY 1 HAVING COUNT(p.conscript_id) = 1 ORDER BY conscripts_count DESC;";
    queries.append(query);

    query.number = "6.3";
    query.description = "Количество медосмотров на каждого призывника";
    query.type = "Lab6";
    query.sqlText = "SELECT p.full_name, (SELECT COUNT(*) FROM public.medical_examinations mo WHERE mo.conscript_id = p.conscript_id) AS examinations_count FROM public.conscripts p ORDER BY examinations_count DESC;";
    queries.append(query);

    query.number = "6.19";
    query.description = "История медосмотров с ФИО и категориями";
    query.type = "Lab6";
    query.sqlText = "SELECT mo.examination_date, mo.doctor_full_name, p.full_name AS conscript_name, kg.category_name FROM public.medical_examinations mo LEFT JOIN public.conscripts p ON p.conscript_id = mo.conscript_id LEFT JOIN public.fitness_categories kg ON kg.category_id = mo.category_id ORDER BY mo.examination_date DESC;";
    queries.append(query);

    // Операции над множествами
    query.number = "6.31";
    query.description = "Все ФИО в системе (UNION)";
    query.type = "Lab6";
    query.sqlText = "SELECT full_name FROM public.conscripts UNION SELECT full_name FROM public.commissioners ORDER BY full_name;";
    queries.append(query);

    return queries;
}
