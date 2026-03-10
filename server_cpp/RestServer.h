#ifndef RESTSERVER_H
#define RESTSERVER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

// В Qt 6.10 TP используем правильные инклюды
#include <QHttpServer>
#include <QHttpServerResponse>

class RestServer : public QObject
{
    Q_OBJECT
public:
    explicit RestServer(QObject *parent = nullptr);
    bool start(quint16 port = 8000);

private:
    QHttpServer m_server;
    void setupRoutes();
};

#endif // RESTSERVER_H