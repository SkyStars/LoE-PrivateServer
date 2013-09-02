#ifndef WIDGET_H
#define WIDGET_H

#include <QtWidgets/QWidget>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QTimer>
#include <QList>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QtConcurrent/QtConcurrentMap>
#include <QCryptographicHash>

#include <windows.h>

#include "character.h"
#include "scene.h"

#define PLAYERSPATH "data/players/"
#define NETDATAPATH "data/netData/"
#define CONFIGFILEPATH "data/server.ini"
#define SERVERSLISTFILEPATH "data/serversList.cfg"
#define TCPPORT 80
#define UDPPORT 80

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

    /// Main functions
public slots:
    void sendCmdLine();
    void checkPingTimeouts();
public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void logMessage(QString msg);
    void logStatusMessage(QString msg);
    void startServer();
    void stopServer();

    /// UDP/TCP
public slots:
    void udpProcessPendingDatagrams();
    void tcpConnectClient();
    void tcpDisconnectClient();
    void tcpProcessPendingDatagrams();
public:
    void tcpProcessData(QByteArray data, QTcpSocket *socket);

public:
    float startTimestamp;
    QUdpSocket *udpSocket;
    QList<Player> tcpPlayers; // Used by the TCP login server
    QList<Player> udpPlayers; // Used by the UDP game server
    QList<Scene> scenes;

private:
    Ui::Widget *ui;
    QTcpServer *tcpServer;
    QList<QTcpSocket *> tcpClientsList;
    QByteArray *tcpReceivedDatas;
    Player& cmdPeer;
    QTimer *pingTimer;

    // Config
    int maxConnected; // Max numbre of players connected at the same time, can deny login
    int maxRegistered; // Max number of registered players in database, can deny registration
    int pingTimeout; // Max time between recdption of pings, can disconnect player
    int pingCheckInterval; // Time between ping timeout checks
    bool logInfos; // Can disable logMessage, but doesn't affect logStatusMessage
    QString saltPassword; // Used to check passwords between login and game servers, must be the same on all the servers involved
    bool enableLoginServer; // Starts a login server
    bool enableGameServer; // Starts a game server
};

// Global import from main
extern Widget win;

#endif // WIDGET_H
