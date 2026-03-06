#ifndef EMBEDDEDQUERIES_H
#define EMBEDDEDQUERIES_H

#include <QList>
#include <QString>

struct QueryInfo {
    QString number;
    QString description;
    QString type;
    QString sqlText;
};

// Функция для получения всех встроенных запросов
QList<QueryInfo> getEmbeddedQueries();

#endif // EMBEDDEDQUERIES_H
