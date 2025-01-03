#include "chatclient.h"

ChatClient::ChatClient(QObject *parent)
    : QObject{parent}, socket{new QTcpSocket(this)}
{
    connect(socket, &QTcpSocket::connected, this, &ChatClient::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ChatClient::onDisconnected);
}

void ChatClient::connectToServer(const QString &host, quint16 port)
{
    socket->connectToHost(host, port);
}

void ChatClient::authenticate(const QString &action, const QString &username, const QString &password)
{
    QJsonObject obj;
    obj["type"] = "AUTH";
    obj["action"] = action; // REGISTER or LOGIN
    obj["username"] = username;
    obj["password"] = password;

    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}
void ChatClient::sendMessage(const QString &to, const QString &content)
{
    QJsonObject obj;
    obj["type"] = "MESSAGE";
    obj["to"] = to;
    obj["content"] = content;
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact) + '\n');

    // Update own message
    m_message << QString("Me to %1: %2").arg(to, content);
    emit messagesChanged();
}


void ChatClient::requestHistory(const QString &withUser)
{
    QJsonObject obj;
    obj["type"] = "GET_HISTORY";
    obj["with"] = withUser;
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
    // qDebug() << obj["with"];
}
void ChatClient::onConnected()
{
    emit connectedToServer();
}
void ChatClient::onReadyRead()
{
    while(socket->canReadLine()) {
        QByteArray data = socket->readLine().trimmed();
        QString message = QString::fromUtf8(data);
        parseMessage(message);
    }
}

void ChatClient::onDisconnected()
{
    emit disconnectedFromServer();
}

QStringList ChatClient::messages() const
{
    return m_message;
}

QStringList ChatClient::users() const
{
    return m_users;
}



void ChatClient::parseMessage(const QString &message)
{

    // Use JSON format to get message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if(!doc.isObject()) {
        // Solve the messages are not JSON format
        if (message.startsWith("ERROR:")) {
            QString errorMsg = message.mid(QString("ERROR:").length());
            emit authenticationResult(false, errorMsg);
        }
        else if (message == "LOGIN_SUCCESS") {
            qDebug() << "Loging success";
            emit authenticationResult(true, "Login successful");
        }
        else if (message == "REGISTER_SUCCESS") {
            qDebug() << "Register sucess";
            emit authenticationResult(true, "Register successful");
        }
        return;
    }
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    if (type == "CLIENT_LIST") {
        QJsonArray userArray = obj["users"].toArray();
        m_users.clear();
        for (const QJsonValue &val: userArray) {
            QJsonObject userObj = val.toObject();
            QString username = userObj["username"].toString();
            QString status = userObj["status"].toString();
            m_users << QString("%1(%2)").arg(username, status);
        }
        emit usersChanged();
    }
    else if(type == "MESSAGE") {
        QString from = obj["from"].toString();
        QString content = obj["content"].toString();
        m_message << QString("%1:%2").arg(from, content);
        // qDebug() << "Message changed";
        emit messagesChanged();
    }
    else if(type == "CHAT_HISTORY") {
        QJsonArray history = obj["history"].toArray();
        m_message.clear();
        for (const QJsonValue &val: history) {
            m_message << val["timestamp"].toString() + " [" + val["sender"].toString() + "] " + val["content"].toString();
        }
        emit messagesChanged();
    }
}
