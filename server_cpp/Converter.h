#ifndef CONVERTER_H
#define CONVERTER_H

#include <QObject>
#include <QStringList>

class Converter : public QObject
{
    Q_OBJECT
public:
    explicit Converter(QObject *parent = nullptr);
    void run();

private:
    QStringList getPrimaryKeyColumns(const QString &tableName);
};

#endif // CONVERTER_H
