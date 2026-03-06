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
    query.sqlText = "SELECT fio as \"ФИО\", data_rozhdeniya as \"Дата рождения\", nomer_pasporta as \"Паспорт\" FROM public.prizivnik;";
    queries.append(query);

    // --- Lab 5 ---
    query.number = "5.1";
    query.description = "Список призывников по возрасту (убывание)";
    query.type = "Lab5";
    query.sqlText = "SELECT fio, data_rozhdeniya, EXTRACT(YEAR FROM AGE(CURRENT_DATE, data_rozhdeniya)) as vozrast FROM public.prizivnik ORDER BY vozrast DESC;";
    queries.append(query);

    query.number = "5.4";
    query.description = "Призывники и их военные билеты";
    query.type = "Lab5";
    query.sqlText = "SELECT p.fio, vb.nomer_bileta, vb.voinskoe_zvanie FROM public.prizivnik p LEFT JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik;";
    queries.append(query);

    query.number = "5.7";
    query.description = "Нагрузка на комиссаров (кол-во призывников)";
    query.type = "Lab5";
    query.sqlText = "SELECT c.fio, COUNT(pc.id_prizivnik) as kolichestvo FROM public.comissar c LEFT JOIN public.prizivnik_comissar pc ON pc.id_comissar = c.id_comissar GROUP BY c.id_comissar, c.fio ORDER BY kolichestvo DESC;";
    queries.append(query);

    // --- Lab 6 ---
    query.number = "6.1";
    query.description = "Категории годности призывников";
    query.type = "Lab6";
    query.sqlText = "SELECT p.fio, kg.nazvanie_kategorii, kg.opisanie_ogranichenii FROM public.prizivnik p JOIN public.voennyi_bilet vb ON vb.id_prizivnika = p.id_prizivnik JOIN public.kategoria_godnosti kg ON kg.id_kategorii = vb.id_kategorii;";
    queries.append(query);

    query.number = "6.3";
    query.description = "Количество медосмотров у каждого призывника";
    query.type = "Lab6";
    query.sqlText = "SELECT p.fio, (SELECT COUNT(*) FROM public.med_osvidetelstvovanie mo WHERE mo.id_prizivnika = p.id_prizivnik) as kol_osmotrov FROM public.prizivnik p;";
    queries.append(query);

    query.number = "6.19";
    query.description = "Журнал медосмотров (Дата/ФИО/Результат)";
    query.type = "Lab6";
    query.sqlText = "SELECT mo.data_provedeniya, p.fio, mo.zaklyuchenie FROM public.med_osvidetelstvovanie mo JOIN public.prizivnik p ON p.id_prizivnik = mo.id_prizivnika ORDER BY mo.data_provedeniya DESC;";
    queries.append(query);

    return queries;
}
