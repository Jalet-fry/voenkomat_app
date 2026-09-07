#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>

class DataGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DataGenerator(QObject *parent = nullptr);
    void run();
};

#endif
