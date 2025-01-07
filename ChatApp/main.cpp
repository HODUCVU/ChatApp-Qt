#include "chatclient.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "QQmlContext"
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ChatClient client;
    client.connectToServer("192.168.43.156", 1234);
    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("chatClient", &client);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("ChatApp", "Main");

    return app.exec();
}
