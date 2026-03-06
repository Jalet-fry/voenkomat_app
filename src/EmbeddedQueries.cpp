#include "EmbeddedQueries.h"
#include <QDebug>

QList<QueryInfo> getEmbeddedQueries()
{
    QList<QueryInfo> queries;
    QueryInfo query;

    // --- Lab 4 ---
    query.number = "4.1";
    query.description = "Общий список всех призывников";
    query.type = "Lab4";
    query.sqlText = "SELECT full_name as \"ФИО\", birth_date as \"Дата рождения\", passport_number as \"Паспорт\" FROM public.conscripts;";
    queries.append(query);

    // --- Lab 5 ---
    query.number = "5.1";
    query.description = "Список призывников по возрасту (убывание)";
    query.type = "Lab5";
    query.sqlText = "SELECT full_name, birth_date, EXTRACT(YEAR FROM AGE(CURRENT_DATE, birth_date)) as vozrast FROM public.conscripts ORDER BY vozrast DESC;";
    queries.append(query);

    query.number = "5.4";
    query.description = "Призывники и их военные билеты";
    query.type = "Lab5";
    query.sqlText = "SELECT p.full_name, vb.ticket_number, vb.military_rank FROM public.conscripts p LEFT JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id;";
    queries.append(query);

    query.number = "5.7";
    query.description = "Нагрузка на комиссаров (кол-во призывников)";
    query.type = "Lab5";
    query.sqlText = "SELECT c.full_name, COUNT(pc.conscript_id) as kolichestvo FROM public.commissioners c LEFT JOIN public.conscripts_commissioners pc ON pc.commissioner_id = c.commissioner_id GROUP BY c.commissioner_id, c.full_name ORDER BY kolichestvo DESC;";
    queries.append(query);

    // --- Lab 6 ---
    query.number = "6.1";
    query.description = "Категории годности призывников";
    query.type = "Lab6";
    query.sqlText = "SELECT p.full_name, kg.category_name, kg.restriction_description FROM public.conscripts p JOIN public.military_id_cards vb ON vb.conscript_id = p.conscript_id JOIN public.fitness_categories kg ON kg.category_id = vb.category_id;";
    queries.append(query);

    query.number = "6.3";
    query.description = "Количество медосмотров у каждого призывника";
    query.type = "Lab6";
    query.sqlText = "SELECT p.full_name, (SELECT COUNT(*) FROM public.medical_examinations mo WHERE mo.conscript_id = p.conscript_id) as kol_osmotrov FROM public.conscripts p;";
    queries.append(query);

    query.number = "6.19";
    query.description = "Журнал медосмотров (Дата/ФИО/Результат)";
    query.type = "Lab6";
    query.sqlText = "SELECT mo.examination_date, p.full_name, mo.conclusion FROM public.medical_examinations mo JOIN public.conscripts p ON p.conscript_id = mo.conscript_id ORDER BY mo.examination_date DESC;";
    queries.append(query);

    return queries;
}
