#include <QApplication>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include "mainwindow.h"

static const char *VISTARA_SERVER_NAME = "VistaraSingleInstance";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Vistara"));
    QApplication::setOrganizationName(QStringLiteral("VistaraOSS"));

    Q_INIT_RESOURCE(vistara);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/vistara_icon_256.png")));

    const QStringList args = app.arguments();
    const QString fileArg = (args.size() >= 2) ? args.at(1) : QString();

    // ---- Try to connect to an existing Vistara instance ----
    {
        QLocalSocket socket;
        socket.connectToServer(QString::fromLatin1(VISTARA_SERVER_NAME));
        if (socket.waitForConnected(100)) {
            // Another instance is running -> send it the file path (if any) and exit.
            if (!fileArg.isEmpty()) {
                const QByteArray data = fileArg.toUtf8();
                socket.write(data);
                socket.flush();
                socket.waitForBytesWritten(500);
            }
            return 0;
        }
    }

    // ---- No other instance: become the primary ----
    QLocalServer server;
    QLocalServer::removeServer(QString::fromLatin1(VISTARA_SERVER_NAME));
    server.listen(QString::fromLatin1(VISTARA_SERVER_NAME));

    MainWindow w;
    w.resize(1024, 768);

    // If a file was passed on startup, open it
    if (!fileArg.isEmpty()) {
        w.openFile(fileArg);
    }

    w.show();

    // When a new process starts and connects, open its file in THIS window
    QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w]() {
        while (QLocalSocket *sock = server.nextPendingConnection()) {
            QObject::connect(sock, &QLocalSocket::readyRead, &w, [sock, &w]() {
                const QByteArray data = sock->readAll();
                const QString path = QString::fromUtf8(data).trimmed();
                if (!path.isEmpty())
                    w.openFile(path);
                sock->disconnectFromServer();
                sock->deleteLater();
            });
        }
    });

    return app.exec();
}
