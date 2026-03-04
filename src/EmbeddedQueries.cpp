#include "EmbeddedQueries.h"
#include "QueriesWindow.h"
#include "DbConstants.h"
#include <QDebug>

QList<QueryInfo> getEmbeddedQueries()
{
    QList<QueryInfo> queries;
    QueryInfo query;
    using namespace Db;

    // --- Lab 4 ---
    query.number = "4.1";
    query.description = "Общий список призывников";
    query.type = "Lab4";
    query.sqlText = QString("SELECT * FROM public.%1;").arg(Tables::CONSCRIPTS);
    queries.append(query);

    // --- Lab 5 ---
    query.number = "5.1";
    query.description = "ФИО и возраст (убывание)";
    query.type = "Lab5";
    query.sqlText = QString("SELECT %1, %2, EXTRACT(YEAR FROM AGE(CURRENT_DATE, %2)) as age FROM public.%3 ORDER BY age DESC;")
        .arg(Conscripts::FULL_NAME).arg(Conscripts::BIRTH_DATE).arg(Tables::CONSCRIPTS);
    queries.append(query);

    query.number = "5.4";
    query.description = "Призывники и их военные билеты";
    query.type = "Lab5";
    query.sqlText = QString("SELECT p.%1, vb.%2, vb.%3 FROM public.%4 p LEFT JOIN public.%5 vb ON vb.%6 = p.%7;")
        .arg(Conscripts::FULL_NAME).arg(MilitaryIdCards::TICKET_NUMBER).arg(MilitaryIdCards::MILITARY_RANK)
        .arg(Tables::CONSCRIPTS).arg(Tables::MILITARY_ID_CARDS).arg(MilitaryIdCards::CONSCRIPT_ID).arg(Conscripts::CONSCRIPT_ID);
    queries.append(query);

    query.number = "5.7";
    query.description = "Количество призывников у комиссаров";
    query.type = "Lab5";
    query.sqlText = QString("SELECT c.%1, COUNT(pc.%2) as count FROM public.%3 c LEFT JOIN public.%4 pc ON pc.%5 = c.%6 GROUP BY c.%6, c.%1 ORDER BY count DESC;")
        .arg(Commissioners::FULL_NAME).arg(ConscriptsCommissioners::CONSCRIPT_ID).arg(Tables::COMMISSIONERS)
        .arg(Tables::CONSCRIPTS_COMMISSIONERS).arg(ConscriptsCommissioners::COMMISSIONER_ID).arg(Commissioners::COMMISSIONER_ID);
    queries.append(query);

    // --- Lab 6 ---
    query.number = "6.1";
    query.description = "Категории годности и возраст";
    query.type = "Lab6";
    query.sqlText = QString("SELECT p.%1, kg.%2, EXTRACT(YEAR FROM AGE(CURRENT_DATE, p.%3)) as age FROM public.%4 p JOIN public.%5 vb ON vb.%6 = p.%7 JOIN public.%8 kg ON kg.%9 = vb.%10;")
        .arg(Conscripts::FULL_NAME).arg(FitnessCategories::CATEGORY_NAME).arg(Conscripts::BIRTH_DATE)
        .arg(Tables::CONSCRIPTS).arg(Tables::MILITARY_ID_CARDS).arg(MilitaryIdCards::CONSCRIPT_ID).arg(Conscripts::CONSCRIPT_ID)
        .arg(Tables::FITNESS_CATEGORIES).arg(FitnessCategories::CATEGORY_ID).arg(MilitaryIdCards::CATEGORY_ID);
    queries.append(query);

    query.number = "6.3";
    query.description = "Количество медосмотров призывника";
    query.type = "Lab6";
    query.sqlText = QString("SELECT p.%1, (SELECT COUNT(*) FROM public.%2 mo WHERE mo.%3 = p.%4) as med_count FROM public.%5 p;")
        .arg(Conscripts::FULL_NAME).arg(Tables::MEDICAL_EXAMINATIONS).arg(MedicalExaminations::CONSCRIPT_ID).arg(Conscripts::CONSCRIPT_ID).arg(Tables::CONSCRIPTS);
    queries.append(query);

    query.number = "6.19";
    query.description = "История медосмотров (Дата/ФИО/Категория)";
    query.type = "Lab6";
    query.sqlText = QString("SELECT mo.%1, p.%2, kg.%3 FROM public.%4 mo JOIN public.%5 p ON p.%6 = mo.%7 JOIN public.%8 kg ON kg.%9 = mo.%10 ORDER BY mo.%1 DESC;")
        .arg(MedicalExaminations::EXAMINATION_DATE).arg(Conscripts::FULL_NAME).arg(FitnessCategories::CATEGORY_NAME)
        .arg(Tables::MEDICAL_EXAMINATIONS).arg(Tables::CONSCRIPTS).arg(Conscripts::CONSCRIPT_ID).arg(MedicalExaminations::CONSCRIPT_ID)
        .arg(Tables::FITNESS_CATEGORIES).arg(FitnessCategories::CATEGORY_ID).arg(MedicalExaminations::CATEGORY_ID);
    queries.append(query);

    return queries;
}
