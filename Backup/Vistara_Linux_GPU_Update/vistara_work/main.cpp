#include <QApplication>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Vistara"));
    app.setOrganizationName(QStringLiteral("VistaraOSS"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/vistara_icon_256.png")));

    QString fileArg;
    for (const QString &arg : app.arguments().mid(1)) {
        if (!arg.startsWith(QLatin1String("--"))) { fileArg = QFileInfo(arg).absoluteFilePath(); break; }
    }

    // Unique per user; Windows/Linux instances must not interfere with one another.
    const QString serverName = QStringLiteral("VistaraSingleInstance_%1")
        .arg(QString::fromLocal8Bit(qgetenv("USER")).isEmpty()
            ? QStringLiteral("desktop") : QString::fromLocal8Bit(qgetenv("USER")));

    QLocalSocket existing;
    existing.connectToServer(serverName);
    if (existing.waitForConnected(120)) {
        if (!fileArg.isEmpty()) {
            existing.write(fileArg.toUtf8());
            existing.flush();
            existing.waitForBytesWritten(1000);
        }
        return 0;
    }

    QLocalServer server;
    QLocalServer::removeServer(serverName);
    server.listen(serverName);
    MainWindow window;
    if (!fileArg.isEmpty()) window.openFile(fileArg);
    window.show();

    QObject::connect(&server, &QLocalServer::newConnection, &window, [&server, &window] {
        while (QLocalSocket *socket = server.nextPendingConnection()) {
            QObject::connect(socket, &QLocalSocket::readyRead, &window, [socket, &window] {
                const QString path = QString::fromUtf8(socket->readAll()).trimmed();
                if (!path.isEmpty()) window.openFile(path);
                window.show();
                window.raise();
                window.activateWindow();
                socket->disconnectFromServer();
                socket->deleteLater();
            });
        }
    });
    return app.exec();
}
