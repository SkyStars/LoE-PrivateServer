#include "widget.h"
#include "ui_widget.h"
#include "character.h"
#include "message.h"
#include "utils.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    cmdPeer((Player&)*(new Player()))
{
    tcpServer = new QTcpServer(this);
    udpSocket = new QUdpSocket(this);
    tcpReceivedDatas = new QByteArray();
    ui->setupUi(this);

    pingTimer = new QTimer(this);
}

Widget::~Widget()
{
    tcpServer->close();

    logMessage(QString("UDP: Disconnecting all players"));
    for (;udpPlayers.size();)
    {
        sendMessage(udpPlayers[0], MsgDisconnect, "Connection closed by the server admin");
        Player::disconnectPlayerCleanup(udpPlayers[0]);
    }

    delete ui;
}

void Widget::logStatusMessage(QString msg)
{
    ui->log->appendPlainText(msg);
    ui->status->setText(msg);
    ui->log->repaint();
    ui->status->repaint();
}

void Widget::logMessage(QString msg)
{
    if (!logInfos)
        return;
    ui->log->appendPlainText(msg);
    ui->log->repaint();
}

void Widget::startServer()
{
    lastNetviewId=0;
    lastId=0;

    /// Read config
    logStatusMessage("Reading config file ...");
    QSettings config(CONFIGFILEPATH, QSettings::IniFormat);;
    maxConnected = config.value("maxConnected",64).toInt();
    maxRegistered = config.value("maxRegistered",2048).toInt();
    pingTimeout = config.value("pingTimeout", 15).toInt();
    pingCheckInterval = config.value("pingCheckInterval", 5000).toInt();
    logInfos = config.value("logInfosMessages", false).toBool();
    saltPassword = config.value("saltPassword", "changeMe").toString();
    enableLoginServer = config.value("enableLoginServer", true).toBool();
    enableGameServer = config.value("enableGameServer", true).toBool();
    enableMultiplayer = config.value("enableMultiplayer", true).toBool();
    syncInterval = config.value("syncInterval",250).toInt();

    /*
    if (config.value("updateGameFiles",false).toBool())
    {
        /// Copy server and game files
        logStatusMessage("Updating game files ...");
        QDir gameDataDir(QDir::homePath()+"/AppData/LocalLow/");
        gameDataDir.mkdir("LoE"); gameDataDir.cd("LoE");
        gameDataDir.mkdir("Legends of Equestria"); gameDataDir.cd("Legends of Equestria");
        gameDataDir.mkdir("data");
        gameDataDir.mkdir("cutiemarks");
        //QString dataPath = QDir::homePath()+"/AppData/LocalLow/LoE/Legends of Equestria/";
        saveResourceToDataFolder("ChatFilter.txt");
        QStringList dataFiles = QDir(":/gameFiles/data").entryList();
        for (int i=0; i<dataFiles.size(); i++)
            saveResourceToDataFolder(QString("data/")+dataFiles[i]);

        QStringList cutimarksFiles = QDir(":/gameFiles/cutiemarks").entryList();
        for (int i=0; i<cutimarksFiles.size(); i++)
            saveResourceToDataFolder(QString("cutiemarks/")+cutimarksFiles[i]);
    }
    */

    /// Init servers
    tcpClientsList.clear();
    startTimestamp=GetTickCount(); // GetTickCount:Nombre de millisecondes depuis le boot (up to 47+ days)

    // Read vortex DB
    if (enableGameServer)
    {
        bool corrupted=false;
        QDir vortexDir("data/vortex/");
        QStringList files = vortexDir.entryList(QDir::Files);
        int nVortex=0;
        for (int i=0; i<files.size(); i++)
        {
            Scene scene(files[i].split('.')[0]);

            QFile file("data/vortex/"+files[i]);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                logStatusMessage("Error reading vortex DB");
                return;
            }
            QByteArray data = file.readAll();
            QList<QByteArray> lines = data.split('\n');
            for (int j=0; j<lines.size(); j++)
            {
                nVortex++;
                Vortex vortex;
                bool ok1, ok2, ok3, ok4;
                QList<QByteArray> elems = lines[j].split(' ');
                if (elems.size() != 5)
                {
                    logStatusMessage("Vortex DB is corrupted. Incorrect line, file " + files[i]);
                    corrupted=true;
                    break;
                }
                vortex.id = elems[0].toInt(&ok1, 16);
                vortex.destName = elems[1];
                vortex.destPos.x = elems[2].toFloat(&ok2);
                vortex.destPos.y = elems[3].toFloat(&ok3);
                vortex.destPos.z = elems[4].toFloat(&ok4);
                if (!(ok1&&ok2&&ok3&&ok4))
                {
                    logStatusMessage("Vortex DB is corrupted. Conversion failed, file " + files[i]);
                    corrupted=true;
                    break;
                }
                scene.vortexes << vortex;
            }
            scenes << scene;
        }

        if (corrupted)
        {
            stopServer();
            return;
        }

        logMessage("Loaded " + QString().setNum(nVortex) + " vortex in " + QString().setNum(scenes.size()) + " scenes");
    }

    // TCP server
    if (enableLoginServer)
    {
        logStatusMessage("Starting TCP login server ...");
        if (!tcpServer->listen(QHostAddress::Any,TCPPORT))
        {
            logStatusMessage("TCP: Unable to start server on port 80");
            stopServer();
            return;
        }
    }

    // UDP server
    if (enableGameServer)
    {
        logStatusMessage("Starting UDP game server ...");
        if (!udpSocket->bind(UDPPORT, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress))
        {
            logStatusMessage("UDP: Unable to start server on port 80");
            stopServer();
            return;
        }
    }

    if (enableLoginServer)
    {
        logStatusMessage("Loading players database ...");
        tcpPlayers = Player::loadPlayers();
    }

    if (enableGameServer)
    {
        // Start ping timeout timer
        pingTimer->start(pingCheckInterval);
    }

    if (enableMultiplayer)
        sync.startSync();

    logStatusMessage("Server started");

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendCmdLine()));
    if (enableLoginServer)
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(tcpConnectClient()));
    if (enableGameServer)
    {
        connect(udpSocket, SIGNAL(readyRead()),this, SLOT(udpProcessPendingDatagrams()));
        connect(pingTimer, SIGNAL(timeout()), this, SLOT(checkPingTimeouts()));
    }
}

void Widget::stopServer()
{
    logStatusMessage("Stopping all server operations");
    tcpServer->close();
    for (int i=0;i<tcpClientsList.size();i++)
        tcpClientsList[i]->close();
    udpSocket->close();

    pingTimer->stop();

    sync.stopSync();

    disconnect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendCmdLine()));
    disconnect(udpSocket, SIGNAL(readyRead()),this, SLOT(udpProcessPendingDatagrams()));
    disconnect(tcpServer, SIGNAL(newConnection()), this, SLOT(tcpConnectClient()));
    disconnect(pingTimer, SIGNAL(timeout()), this, SLOT(checkPingTimeouts()));
}

void Widget::sendCmdLine()
{
    if (!enableGameServer)
    {
        logMessage("This is not a game server, commands are disabled");
        return;
    }

    QString str = ui->cmdLine->text();

    if (str.startsWith("setPeer"))
    {
        if (udpPlayers.size() == 1)
        {
            cmdPeer = udpPlayers[0];
            QString peerName = cmdPeer.IP + " " + QString().setNum(cmdPeer.port);
            logMessage(QString("UDP: Peer set to ").append(peerName));
            return;
        }

        str = str.right(str.size()-8);
        QStringList args = str.split(' ');
        if (args.size() != 2)
        {
            logMessage("UDP: setPeer takes two arguments");
            return;
        }
        bool ok;
        quint16 port = args[1].toUInt(&ok);
        if (!ok)
        {
            logMessage("UDP: setPeer takes a port as argument");
            return;
        }

        cmdPeer = Player::findPlayer(udpPlayers,args[0], port);
        if (cmdPeer.IP!="")
            logMessage(QString("UDP: Peer set to ").append(str));
        else
            logMessage(QString("UDP: Peer not found (").append(str).append(")"));
        return;
    }
    else if (str.startsWith("listPeers"))
    {
        if (str.size()<=10)
        {
            for (int i=0; i<win.udpPlayers.size();i++)
                win.logMessage(win.udpPlayers[i].IP
                               +":"+QString().setNum(win.udpPlayers[i].port)
                               +" "+QString().setNum((int)timestampNow()-win.udpPlayers[i].lastPingTime)+"s");
            return;
        }
        str = str.right(str.size()-10);
        Scene* scene = findScene(str);
        if (scene->name.isEmpty())
            win.logMessage("Can't find scene");
        else
            for (int i=0; i<scene->players.size();i++)
                win.logMessage(win.udpPlayers[i].IP
                               +":"+QString().setNum(win.udpPlayers[i].port)
                               +" "+QString().setNum((int)timestampNow()-win.udpPlayers[i].lastPingTime)+"s");
        return;
    }
    else if (str.startsWith("sync"))
    {
        win.logMessage("UDP: Syncing manually");
        sync.doSync();
        return;
    }

    if (cmdPeer.IP=="")
    {
        logMessage("Select a peer first with setPeer/listPeers");
        return;
    }
    else // Refresh peer info
    {
        cmdPeer = (Player&)Player::findPlayer(udpPlayers,cmdPeer.IP, cmdPeer.port);
        if (cmdPeer.IP=="")
        {
            logMessage(QString("UDP: Peer not found"));
            return;
        }
    }

    if (str.startsWith("disconnect"))
    {
        logMessage(QString("UDP: Disconnecting"));
        sendMessage(cmdPeer,MsgDisconnect, "Connection closed by the server admin");
        Player::disconnectPlayerCleanup(cmdPeer); // Save game and remove the player
    }
    else if (str.startsWith("load"))
    {
        str = str.right(str.size()-5);
        sendLoadSceneRPC(cmdPeer, str);
    }
    else if (str.startsWith("getPos"))
    {
        logMessage(QString("Pos : x=") + QString().setNum(cmdPeer.pony.pos.x)
                   + ", y=" + QString().setNum(cmdPeer.pony.pos.y)
                   + ", z=" + QString().setNum(cmdPeer.pony.pos.z));
    }
    else if (str.startsWith("sendPonies"))
    {
        sendPonies(cmdPeer);
    }
    else if (str.startsWith("sendUtils3"))
    {
        logMessage("UDP: Sending Utils3 request");
        QByteArray data(1,3);
        sendMessage(cmdPeer,MsgUserReliableOrdered6,data);
    }
    else if (str.startsWith("setPlayerId"))
    {
        str = str.right(str.size()-12);
        QByteArray data(3,4);
        bool ok;
        unsigned id = str.toUInt(&ok);
        if (ok)
        {
            logMessage("UDP: Sending setPlayerId request");
            data[1]=id;
            data[2]=id >> 8;
            sendMessage(cmdPeer,MsgUserReliableOrdered6,data);
        }
        else
            logStatusMessage("Error : Player ID is a number");
    }
    else if (str.startsWith("remove"))
    {
        str = str.right(str.size()-7);
        QByteArray data(3,2);
        bool ok;
        unsigned id = str.toUInt(&ok);
        if (ok)
        {
            logMessage("UDP: Sending remove request");
            data[1]=id;
            data[2]=id >> 8;
            sendMessage(cmdPeer,MsgUserReliableOrdered6,data);
        }
        else
            logStatusMessage("Error : Remove needs the id of the view to remove");
    }
    else if (str.startsWith("sendPonyData"))
    {
        QByteArray data(3,0xC8);
        data[0] = cmdPeer.pony.netviewId;
        data[1] = cmdPeer.pony.netviewId>>8;
        data += cmdPeer.pony.ponyData;
        sendMessage(cmdPeer, MsgUserReliableOrdered18, data);
        return;
    }
    else if (str.startsWith("setStat"))
    {
        str = str.right(str.size()-8);
        QStringList args = str.split(' ');
        if (args.size() != 2)
        {
            logStatusMessage("Error : usage is setState StatID StatValue");
            return;
        }
        bool ok,ok2;
        quint8 statID = args[0].toInt(&ok);
        float statValue = args[1].toFloat(&ok2);
        if (!ok || !ok2)
        {
            logStatusMessage("Error : usage is setState StatID StatValue");
            return;
        }
        sendSetStatRPC(cmdPeer, statID, statValue);
    }
    else if (str.startsWith("setMaxStat"))
    {
        str = str.right(str.size()-11);
        QStringList args = str.split(' ');
        if (args.size() != 2)
        {
            logStatusMessage("Error : usage is setMaxStat StatID StatValue");
            return;
        }
        bool ok,ok2;
        quint8 statID = args[0].toInt(&ok);
        float statValue = args[1].toFloat(&ok2);
        if (!ok || !ok2)
        {
            logStatusMessage("Error : usage is setMaxState StatID StatValue");
            return;
        }
        sendSetMaxStatRPC(cmdPeer, statID, statValue);
    }
    else if (str.startsWith("instantiate"))
    {
        if (str == "instantiate")
        {
            logMessage("UDP: Instantiating");
            sendNetviewInstantiate(cmdPeer);
            return;
        }

        QByteArray data(1,1);
        str = str.right(str.size()-12);
        QStringList args = str.split(' ');

        if (args.size() != 3 && args.size() != 6 && args.size() != 10)
        {
            logStatusMessage(QString("Error : Instantiate takes 0,3,6 or 10 arguments").append(str));
            return;
        }
        // Au as au moins les 3 premiers de toute facon
        data += stringToData(args[0]);
        unsigned viewId, ownerId;
        bool ok1, ok2;
        viewId = args[1].toUInt(&ok1);
        ownerId = args[2].toUInt(&ok2);
        if (!ok1 || !ok2)
        {
            logStatusMessage(QString("Error : instantiate key viewId ownerId x1 y1 z1 x2 y2 z2 w2"));
            return;
        }
        QByteArray params1(4,0);
        params1[0] = viewId;
        params1[1] = viewId >> 8;
        params1[2] = ownerId;
        params1[3] = ownerId >> 8;
        data += params1;
        float x1=0,y1=0,z1=0;
        float x2=0,y2=0,z2=0,w2=0;
        if (args.size() >= 6) // Si on a le vecteur position on l'ajoute
        {
            bool ok1, ok2, ok3;
            x1=args[3].toFloat(&ok1);
            y1=args[4].toFloat(&ok2);
            z1=args[5].toFloat(&ok3);

            if (!ok1 || !ok2 || !ok3)
            {
                logStatusMessage(QString("Error : instantiate key viewId ownerId x1 y1 z1 x2 y2 z2 w2"));
                return;
            }
        }
        data+=floatToData(x1);
        data+=floatToData(y1);
        data+=floatToData(z1);

        if (args.size() == 10) // Si on a le quaternion rotation on l'ajoute
        {
            bool ok1, ok2, ok3, ok4;
            x2=args[6].toFloat(&ok1);
            y2=args[7].toFloat(&ok2);
            z2=args[8].toFloat(&ok3);
            w2=args[9].toFloat(&ok4);

            if (!ok1 || !ok2 || !ok3 || !ok4)
            {
                logStatusMessage(QString("Error : instantiate key viewId ownerId x1 y1 z1 x2 y2 z2 w2"));
                return;
            }
        }
        data+=floatToData(x2);
        data+=floatToData(y2);
        data+=floatToData(z2);
        data+=floatToData(w2);

        logMessage(QString("UDP: Instantiating ").append(args[0]));
        sendMessage(cmdPeer,MsgUserReliableOrdered6,data);
    }
    else if (str.startsWith("beginDialog"))
    {
        QByteArray data(1,0);
        data[0] = 11; // Request number

        sendMessage(cmdPeer,MsgUserReliableOrdered4, data);
    }
    else if (str.startsWith("endDialog"))
    {
        QByteArray data(1,0);
        data[0] = 13; // Request number

        sendMessage(cmdPeer,MsgUserReliableOrdered4, data);
    }
    else if (str.startsWith("setDialogMsg"))
    {
        str = str.right(str.size()-13);
        QByteArray data(1,0);
        data[0] = 0x11; // Request number

        data += stringToData(str);

        sendMessage(cmdPeer,MsgUserReliableOrdered4, data);
    }
    else if (str.startsWith("move"))
    {
        str = str.right(str.size()-5);
        QByteArray data(1,0);
        data[0] = 0xce; // Request number

        // Serialization : float x, float y, float z

        QStringList coords = str.split(' ');
        if (coords.size() != 3)
            return;

        logMessage(QString("UDP: Moving character"));

        QByteArray x = floatToData(coords[0].toFloat());
        QByteArray y = floatToData(coords[1].toFloat());
        QByteArray z = floatToData(coords[2].toFloat());

        data += x;
        data += y;
        data += z;

        sendMessage(cmdPeer,MsgUserReliableOrdered4, data);
    }
    else if (str.startsWith("error"))
    {
        str = str.right(str.size()-6);
        QByteArray data(1,0);
        data[0] = 0x7f; // Request number

        data += stringToData(str);

        sendMessage(cmdPeer,MsgUserReliableOrdered4, data);
    }
}
