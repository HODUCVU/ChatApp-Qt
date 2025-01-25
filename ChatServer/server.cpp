#include "server.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent)
{
    if(!initializeDatabase()) {
        qDebug() << "Failed to initalize Database";
        exit(EXIT_FAILURE);
    }
}

bool ChatServer::initializeDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("chat_server.db");
    if(!db.open()) {
        qDebug() << "Cannot open database: " << db.lastError();
        return false;
    }
    QSqlQuery query;
    // Create user info table
    query.exec("CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "username TEXT UNIQUE,"
               "password TEXT,"
               "status TEXT)");
    // Create message table
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "sender_id INTEGER, "
               "receiver_id INTEGER, "
               "content TEXT, "
               "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "FOREIGN KEY(sender_id) REFERENCES users(id), "
               "FOREIGN KEY(receiver_id) REFERENCES users(id))");

    // QSqlQuery query;
    // query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='users';");
    // query.exec();
    // if(query.next()) {
    //     qDebug()<< "Table 'users' exists";
    // } else
    //     qDebug() << "Table 'users' does not exist";

    // query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='messages';");
    // query.exec();
    // if(query.next()) {
    //     qDebug() << "Table 'messages' exists.";
    // } else {
    //     qDebug() << "Table 'messages' does not exist.";

    // }

    return true;
}
bool ChatServer::startServer(quint16 port)
{
    // QString host = "192.168.19.24";
    if(!listen(QHostAddress::Any, port)) {
    // if(!listen(QHostAddress(host), port)) {
        qDebug() << "Server couldn't start on port:" << port;
        return false;
    }
    qDebug() << "Server started on port:" << port;
    return true;
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *clientSocket = new QTcpSocket(this);
    if(clientSocket->setSocketDescriptor(socketDescriptor)) {
        ClientInfo info;
        info.socket = clientSocket;
        info.username = "Unknown";
        clients.insert(clientSocket, info);
        // connect(clientSocket, &QTcpSocket::readyRead, this, &ChatServer::onReadyRead);
        // connect(clientSocket, &QTcpSocket::disconnected, this, &ChatServer::onDisconnected);
        connect(info.socket, &QTcpSocket::readyRead, this, &ChatServer::onReadyRead);
        connect(info.socket, &QTcpSocket::disconnected, this, &ChatServer::onDisconnected);

        qDebug() << "New client connected: " << socketDescriptor;
    } else {
        delete clientSocket;
    }
}

void ChatServer::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if(!client)
        return;
    while(client->canReadLine()) {
        QByteArray data = client->readLine().trimmed();
        QString message = QString::fromUtf8(data);
        // qDebug() << "Received from " << clients[client].username << ":" << message;
        handleMessage(client, message);
    }
}

void ChatServer::onDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if(!client)
        return;
    QString username = clients[client].username;
    clients.remove(client);
    client->deleteLater();
    qDebug() << "Client " << username << " disconnected.";

    // Update user status
    if(!username.isEmpty()) {
        QSqlQuery query;
        query.prepare("UPDATE users SET status=:status WHERE username=:username");
        query.bindValue(":status", "offline");
        query.bindValue(":username", username);
        query.exec();
        sendClientList();
    }
}

void ChatServer::handleMessage(QTcpSocket *client, const QString &message)
{
    // Use JSON format to send message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if(!doc.isObject()) {
        sendMessageToClient(client, "ERROR:Invalid message format");
        return;
    }
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    if(type == "AUTH") {
        // Authenticate user: Register or Login
        QString action = obj["action"].toString();
        QString username = obj["username"].toString();
        QString password = obj["password"].toString();

        qDebug() << username << " " << password;

        QStringList credentials = {action, username, password};
        QString authResult = authenticateUser(client, credentials);
        if(!authResult.isEmpty()) {
            clients[client].username = authResult;
            sendClientList();
        }
    } else if (type == "MESSAGE") {
        // Enter message to send user
        // qDebug() << "SEND MESSAGE";
        QString to = obj["to"].toString(); // send to
        QString content = obj["content"].toString();
        // qDebug() << "To: " << to << " -- content: " << content;
        // Get sender id and receiver id
        QSqlQuery query;
        query.prepare("SELECT id FROM users WHERE username=:username");
        query.bindValue(":username", clients[client].username);
        query.exec();
        int senderId = query.next() ? query.value(0).toInt() : -1;
        // qDebug() << "Sender Id: " << senderId;
        query.prepare("SELECT id FROM users WHERE username=:username");
        query.bindValue(":username", to);
        query.exec();
        int receiverId = query.next() ? query.value(0).toInt() : - 1;
        qDebug() << "receiver Id: " << receiverId;
        if(senderId == -1 || receiverId == -1) {
            sendMessageToClient(client, "ERROR:Invalid recipient");
            return;
        }
        // Save message to messages database
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO messages (sender_id, receiver_id, content) "
                            "VALUES (:sender_id, :receiver_id, :content)");
        insertQuery.bindValue(":sender_id", senderId);
        insertQuery.bindValue(":receiver_id", receiverId);
        insertQuery.bindValue(":content", content);
        if(!insertQuery.exec()) {
            qDebug() << "insert message error";
        }

        // Send message to receiver if online
        foreach(QTcpSocket *socket, clients.keys()) {
            if(clients[socket].username == to) {
                QJsonObject msg;
                msg["type"] = "MESSAGE";
                msg["from"] = clients[client].username;
                msg["content"] = content;
                QJsonDocument msgDoc(msg);
                sendMessageToClient(socket, QString::fromUtf8(msgDoc.toJson(QJsonDocument::Compact)));
            }
        }
    }
    else if (type == "GET_HISTORY")
    {
        // Request history chat
        QString withUser = obj["with"].toString();
        sendChatHistory(client, withUser);
    }
}
QString ChatServer::authenticateUser(QTcpSocket *client, const QStringList &credentials)
{
    if(credentials.size() < 3) {
        sendMessageToClient(client, "ERROR:Invalid authentication format");
        return QString();
    }
    QString action = credentials[0];
    QString username = credentials[1];
    QString password = credentials[2];
    // qDebug() << "Action: " << action << " Username: " << username << " Password: " << password;
    QSqlQuery query;
    if(action == "REGISTER") {
        query.prepare("INSERT INTO users (username, password, status) VALUES (:username, :password, :status)");
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        query.bindValue(":status", "offline");
        if(!query.exec()) {
            qDebug() << "Failed exec";
            qDebug() << "SQL Error during registration:" << query.lastError().text();

            sendMessageToClient(client, "ERORR:Registration falied. Username may already exist.");
            return QString();
        }
        sendMessageToClient(client, "REGISTER_SUCCESS");
        return username;
    } else if (action == "LOGIN") {
        query.prepare("SELECT id FROM users WHERE username = :username AND password = :password");
        query.bindValue(":username", username);
        query.bindValue(":password", password);
        if(!query.exec())
            return QString();
        if(query.next()) {
            // Update status to online
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE users SET status=:status WHERE username=:username");
            updateQuery.bindValue(":status", "online");
            updateQuery.bindValue(":username", username);
            updateQuery.exec();
            sendMessageToClient(client, "LOGIN_SUCCESS");
            return username;
        } else {
            sendMessageToClient(client, "ERROR:Invalid username or password");
            return QString();
        }
    }
    return QString();
}

void ChatServer::sendMessageToClient(QTcpSocket *client, const QString &message)
{
    client->write(message.toUtf8() + "\n");
}

void ChatServer::sendClientList()
{
    QJsonObject obj;
    obj["type"] = "CLIENT_LIST";
    QJsonArray array;
    foreach(const ClientInfo &info, clients) {
        if(info.username != "Unknown") {
            QJsonObject userObj;
            userObj["username"] = info.username;
            userObj["status"] = "online";
            array.append(userObj);
        }
    }
    QSqlQuery query;
    query.prepare("SELECT username, status FROM users WHERE status=:status");
    query.bindValue(":status", "offline");
    query.exec();
    while (query.next()) {
        QJsonObject userObj;
        userObj["username"] = query.value(0).toString();
        userObj["status"] = query.value(1).toString();
        array.append(userObj);
    }
    obj["users"] = array;
    QJsonDocument doc(obj);
    QString msg = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    broadcastMessage(msg);
}

void ChatServer::broadcastMessage(const QString &message, QTcpSocket *excludeClient)
{
    foreach(QTcpSocket *client, clients.keys()) {
        if(client != excludeClient && clients[client].username != "Unknown") {
            sendMessageToClient(client, message);
        }
    }
}

void ChatServer::sendChatHistory(QTcpSocket *client, const QString &withUser)
{
    QJsonObject obj;
    obj["type"] = "CHAT_HISTORY";

    // Get both id of two user
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username=:username");
    query.bindValue(":username", clients[client].username);
    query.exec();
    int userId = query.next() ? query.value(0).toInt() : -1;

    query.prepare("SELECT id FROM users WHERE username=:username");
    query.bindValue(":username", withUser);
    query.exec();
    int withUserId = query.next() ? query.value(0).toInt() : -1;
    // qDebug() << "UserId: " << userId << " withUserId: " << withUserId;
    if(userId == -1 || withUserId == -1)
    {
        // obj["history"] = QStringList();
        obj["history"] = QJsonArray();
    } else {
        // Get message from both user
        query.prepare("SELECT u1.username, m.content, m.timestamp "
                      "FROM messages m "
                      "JOIN users u1 ON m.sender_id = u1.id "
                      "WHERE (m.sender_id=:userId AND m.receiver_id=:withUserId) "
                      "OR (m.sender_id=:withUserId AND m.receiver_id=:userId) "
                      "ORDER BY m.timestamp ASC");

        query.bindValue(":userId", userId);
        query.bindValue(":withUserId", withUserId);

        if (!query.exec()) {
            qDebug() << "Error fetching history:" << query.lastError().text();
            return;
        }
        QJsonArray history;
        while(query.next()) {
            QString sender = query.value(0).toString();
            QString content = query.value(1).toString();
            QString timestamp = query.value(2).toString();
            QJsonObject messageObj;
            messageObj["sender"] = sender;
            messageObj["content"] = content;
            messageObj["timestamp"] = timestamp;
            // history << QString("[%1]%2%3").arg(timestamp,sender,content);
            history.append(messageObj);
        }
        obj["history"] = history;
    }
    QJsonDocument doc(obj);
    QString msg = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    sendMessageToClient(client, msg);
}
