#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ChatClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList messages READ messages NOTIFY messagesChanged FINAL)
    Q_PROPERTY(QStringList users READ users NOTIFY usersChanged FINAL)
public:
    explicit ChatClient(QObject *parent = nullptr);
    Q_INVOKABLE void connectToServer(const QString &host, quint16 port);
    Q_INVOKABLE void sendMessage(const QString& to, const QString &content);
    Q_INVOKABLE void authenticate(const QString &action, const QString &username, const QString &password);
    Q_INVOKABLE void requestHistory(const QString &withUser);

    QStringList messages() const;
    QStringList users() const;
signals:
    void messagesChanged();
    void usersChanged();
    void connectedToServer();
    void disconnectedFromServer();
    void authenticationResult(bool success, const QString &message);
private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
private:
    QTcpSocket *socket;
    QStringList m_message;
    QStringList m_users;
    void parseMessage(const QString& message);
};

#endif // CHATCLIENT_H
