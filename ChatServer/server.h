#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QtSql/QSqlDatabase>
#include <QMap>

struct ClientInfo {
    QString username;
    QTcpSocket *socket;
};

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    ChatServer(QObject *parent = nullptr);
    bool startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
private slots:
    void onReadyRead();
    void onDisconnected();
private:
    QSqlDatabase db;
    QMap<QTcpSocket*, ClientInfo> clients;
    bool initializeDatabase();
    void handleMessage(QTcpSocket *client, const QString &message);
    void sendClientList();
    void sendMessageToClient(QTcpSocket *client, const QString &message);
    void broadcastMessage(const QString &message, QTcpSocket *excludeClient = nullptr);
    QString authenticateUser(QTcpSocket *client, const QStringList &credentials);
    void sendChatHistory(QTcpSocket *client, const QString &withUser);
};

#endif // SERVER_H
