#include "widget.h"
#include "message.h"
#include "utils.h"

void Widget::tcpConnectClient()
{
    logMessage("TCP : New client connected");

    QTcpSocket *newClient = tcpServer->nextPendingConnection();
    tcpClientsList << newClient;

    connect(newClient, SIGNAL(readyRead()), this, SLOT(tcpProcessPendingDatagrams()));
    connect(newClient, SIGNAL(disconnected()), this, SLOT(tcpDisconnectClient()));
}

void Widget::tcpDisconnectClient()
{
    // On détermine quel client se déconnecte
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket == 0) // Si par hasard on n'a pas trouvé le client à l'origine du signal, on arrête la méthode
    return;

    logMessage("TCP : Client disconnected");
    disconnect(socket);

    tcpClientsList.removeOne(socket);

    socket->deleteLater();
    socket=0;
}

void Widget::tcpProcessPendingDatagrams()
{
    // On détermine quel client envoie le message (recherche du QTcpSocket du client)
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket == 0) // Si par hasard on n'a pas trouvé le client à l'origine du signal, on arrête la méthode
        return;

    unsigned nTries = 0;

    // Acquire data
    while(socket->state()==QAbstractSocket::ConnectedState && nTries<3) // Exit if disconnected, too much retries, malformed HTTP request, or after all requests are processed
    {
        // TODO: Give up after three tries reading the same message
        tcpReceivedDatas->append(socket->readAll());
        nTries++;

        if (!tcpReceivedDatas->startsWith("POST") && !tcpReceivedDatas->startsWith("GET")) // Not HTTP, clear the buffer
        {
            logMessage("TCP : Received non-HTTP request");
            tcpReceivedDatas->clear();
            socket->close();
            return;
        }
        else if (tcpReceivedDatas->contains("Content-Length:")) // POST or GET request, wait for Content-Length header
        {
            QByteArray contentLength = *tcpReceivedDatas;
            contentLength = contentLength.right(contentLength.size() - contentLength.indexOf("Content-Length:") - 15);
            QList<QByteArray> lengthList = contentLength.trimmed().split('\n');
            if (lengthList.size()>1) // We want a number on this line and a next line to be sure we've got the full number
            {
                bool isNumeric;
                int length = lengthList[0].trimmed().toInt(&isNumeric);
                if (!isNumeric) // We've got something but it's not a number
                {
                    logMessage("TCP : Content-Length must be a decimal number");
                    tcpReceivedDatas->clear();
                    socket->close();
                    return;
                }

                // Get the HTML data/payload
                QByteArray data = *tcpReceivedDatas;
                data = removeHTTPHeader(data, "POST ");
                data = removeHTTPHeader(data, "GET ");
                data = removeHTTPHeader(data, "User-Agent:");
                data = removeHTTPHeader(data, "Host:");
                data = removeHTTPHeader(data, "Accept:");
                data = removeHTTPHeader(data, "Content-Length:");
                data = removeHTTPHeader(data, "Content-Type:");
                data = removeHTTPHeader(data, "Server:");
                data = removeHTTPHeader(data, "Date:");
                data = removeHTTPHeader(data, "Transfert-Encoding:");
                data = removeHTTPHeader(data, "Connection:");
                data = removeHTTPHeader(data, "Vary:");
                data = removeHTTPHeader(data, "X-Powered-By:");
                data = removeHTTPHeader(data, "accept-encoding:");
                data = removeHTTPHeader(data, "if-modified-since:");

                if (data.size() >= length) // Wait until we have all the data
                {
                    data.truncate(length);

                    // Process data, if the buffer is not empty, keep reading
                    tcpProcessData(data, socket);
                    if (tcpReceivedDatas->isEmpty())
                        return;
                    nTries=0;
                }
            }
        }
    }
}

void Widget::tcpProcessData(QByteArray data, QTcpSocket* socket)
{
    if (tcpReceivedDatas->contains("commfunction=login&") && tcpReceivedDatas->contains("&version=")) // Login request
    {
        QString postData = QString(*tcpReceivedDatas);
        *tcpReceivedDatas = tcpReceivedDatas->right(postData.size()-postData.indexOf("version=")-8-4); // 4 : size of version number (ie:version=1344)
        logMessage("TCP : Login request received :");
        QFile file(QString(NETDATAPATH)+"/loginHeader.bin");
        QFile fileServersList(SERVERSLISTFILEPATH);
        QFile fileBadPassword(QString(NETDATAPATH)+"/loginWrongPassword.bin");
        QFile fileAlready(QString(NETDATAPATH)+"/loginAlreadyConnected.bin");
        QFile fileMaxRegistration(QString(NETDATAPATH)+"/loginMaxRegistered.bin");
        QFile fileMaxConnected(QString(NETDATAPATH)+"/loginMaxConnected.bin");
        if (!file.open(QIODevice::ReadOnly) || !fileBadPassword.open(QIODevice::ReadOnly)
        || !fileAlready.open(QIODevice::ReadOnly) || !fileMaxRegistration.open(QIODevice::ReadOnly)
        || !fileMaxConnected.open(QIODevice::ReadOnly) || !fileServersList.open(QIODevice::ReadOnly))
        {
            logStatusMessage("Error reading login data files");
            stopServer();
        }
        else
        {
            bool ok=true;
            postData = postData.right(postData.size()-postData.indexOf("username=")-9);
            QString username = postData;
            username.truncate(postData.indexOf('&'));
            postData = postData.right(postData.size()-postData.indexOf("passhash=")-9);
            QString passhash = postData;
            passhash.truncate(postData.indexOf('&'));
            logMessage(QString("IP : ")+socket->peerAddress().toString());
            logMessage(QString("Username : ")+username);
            logMessage(QString("Passhash : ")+passhash);

            // Add player to the players vector
            // TODO: If there is already a player with that name, check the passhashes
            Player& player = Player::findPlayer(tcpPlayers, username);
            if (player.name != username) // Not found, create a new player
            {
                // Check max registered number
                if (tcpPlayers.size() >= maxRegistered)
                {
                    logMessage("TCP : Registration failed, too much players registered");
                    socket->write(fileMaxRegistration.readAll());
                    ok = false;
                }
                else
                {
                    logMessage("TCP : Creating user in database");
                    Player newPlayer;
                    newPlayer.name = username;
                    newPlayer.passhash = passhash;
                    newPlayer.IP = socket->peerAddress().toString();
                    newPlayer.connected = false; // The connection checks are done by the game servers
                    newPlayer.lastPingTime = timestampNow();
                    newPlayer.lastPingNumber = 0;
                    for (int i=0;i<32;i++)
                        newPlayer.udpSequenceNumbers[i]=0;

                    tcpPlayers << newPlayer;
                    if (!Player::savePlayers(tcpPlayers))
                        ok = false;
                }
            }
            else // Found, compare passhashes, check if already connected
            {
                if (player.passhash != passhash) // Bad password
                {
                    logMessage("TCP : Login failed, wrong password");
                    socket->write(fileBadPassword.readAll());
                    ok=false;
                }
                /*
                else if (newPlayer.connected) // Already connected
                {
                    logMessage("TCP : Login failed, player already connected");
                    socket->write(fileAlready.readAll());
                    ok=false;
                }
                */
                else // Good password
                {
                    /*
                    // Check too many connected
                    int n=0;
                    for (int i=0;i<tcpPlayers.size();i++)
                        if (tcpPlayers[i].connected)
                            n++;
                    if (n>=maxConnected)
                    {
                        logMessage("TCP : Login failed, too much players connected");
                        socket->write(fileMaxConnected.readAll());
                        ok=false;
                    }
                    else
                    */
                    {
                        player.reset();
                        player.IP = socket->peerAddress().toString();
                        player.lastPingTime = timestampNow();
                        player.connected = true;
                    }
                }
            }

            if (ok) // Send servers list
            {
                QByteArray customData = file.readAll();

                QByteArray data1 = QByteArray::fromHex("0D0A61757468726573706F6E73653A0A747275650A");
                QByteArray sesskey = QCryptographicHash::hash(QString(passhash + saltPassword).toLatin1(), QCryptographicHash::Md5).toHex();
                sesskey += passhash;
                QByteArray data2 = QByteArray::fromHex("0A310A");
                QByteArray serversList;
                QByteArray line;
                do {
                    line = fileServersList.readLine().trimmed();
                    serversList+=line;
                    serversList+=0x0A;
                } while (!line.isEmpty());
                serversList = serversList.trimmed();
                QByteArray data3 = QByteArray::fromHex("0D0A300D0A0D0A");
                int dataSize = data1.size() + sesskey.size() + data2.size() + serversList.size() - 2;
                QString dataSizeStr = QString().setNum(dataSize, 16);

                customData += dataSizeStr;
                customData += data1;
                customData += sesskey;
                customData += data2;
                customData += serversList;
                customData += data3;

                logMessage("TCP : Login successful, sending servers list");
                socket->write(customData);
            }
        }
    }
    else if (data.contains("commfunction=removesession"))
    {
        logMessage("TCP : Session closed by client");
    }
    else // Unknown request, erase tcp buffer
    {
        // Display data
        logMessage("TCP : Unknow request received : ");
        logMessage(QString(data.data()));
    }

    // Delete the processed message from the buffer
    *tcpReceivedDatas = tcpReceivedDatas->right(tcpReceivedDatas->size() - tcpReceivedDatas->indexOf(data) - data.size());
}
