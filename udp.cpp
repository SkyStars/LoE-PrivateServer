#include "widget.h"
#include "message.h"
#include "utils.h"

void Widget::udpProcessPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QHostAddress rAddr;
        quint16 rPort;
        QByteArray datagram;
        qint64 dataRead = 0;
        int datagramSize = udpSocket->pendingDatagramSize();
        datagram.resize(datagramSize);

        while(dataRead < datagram.size())
        {
            qint64 readNow = udpSocket->readDatagram(datagram.data()+dataRead, datagramSize, &rAddr, &rPort); // le +dataRead sur un tableau, ça décale le pointeur d'un offset de taille dataRead !
            if(readNow != -1)
            {
                dataRead += readNow; // Compte le nombre d'octets lus au total, s'arreter quand dataRead atteint pendingDatagramSize
                if (datagramSize > (datagram.size() - dataRead)) // Evite de lire après la fin du tableau en mode fragmenté, evite donc que dataSent dépasse datagramSize, sinon Overflow et envoi de données inutiles et aléatoires !!
                    datagramSize = (datagram.size() - dataRead);
                //QMessageBox::information(NULL,"SenderInfo",QString("DatagramSize : %1  sentNow : %2  dataSent : %3 Sizeof(datagram->data()) : %4").arg(QString().setNum(datagramSize),QString().setNum(sentNow),QString().setNum(dataSent),QString().setNum(datagram->size())));
            }
            else
            {
                logStatusMessage(QString("Socket error : ").arg(udpSocket->errorString()));
                return;
            }
        }

        // Add player on connection
        if ((unsigned char)datagram[0]==MsgConnect && (unsigned char)datagram[1]==0 && (unsigned char)datagram[2]==0 && datagram.size()>=22)
        {
            QString name = dataToString(datagram.right(datagram.size()-22));
            QString sesskey = dataToString(datagram.right(datagram.size()-datagram.lastIndexOf(name)-name.size()));
            //logMessage(QString("UDP: Connect detected with name : ")+name);
            //logMessage(QString("UDP: Connect detected with sesskey : ")+sesskey);

            if (QCryptographicHash::hash(QString(sesskey.right(40) + saltPassword).toLatin1(), QCryptographicHash::Md5).toHex() == sesskey.left(32))
            {
                //logMessage("Sesskey token accepted");

                // Create new player if needed, else just update player
                Player& newPlayer = Player::findPlayer(udpPlayers, rAddr.toString(),rPort);
                if (newPlayer.IP != rAddr.toString()) // IP:Port not found in player list
                {
                    newPlayer.reset();
                    newPlayer.connected = true;
                    newPlayer.name = name;
                    newPlayer.IP = rAddr.toString();
                    newPlayer.port = rPort;

                    // Check if we have too many players connected
                    int n=0;
                    for (int i=0;i<udpPlayers.size();i++)
                        if (udpPlayers[i].connected)
                            n++;
                    if (n>=maxConnected)
                    {
                        sendMessage(newPlayer, MsgDisconnect, "Error : Too much players connected. Try again later.");
                    }

                    // If not add the player
                    udpPlayers << newPlayer;
                }
                else  // IP:Port found in player list
                {
                    if (newPlayer.connected) // TODO: Error, player already connected
                    {
                        sendMessage(newPlayer, MsgDisconnect, "Error : Player already connected.");
                        return;
                    }

                    // Check if we have too many players connected
                    int n=0;
                    for (int i=0;i<udpPlayers.size();i++)
                        if (udpPlayers[i].connected)
                            n++;
                    if (n>=maxConnected)
                    {
                        sendMessage(newPlayer, MsgDisconnect, "Error : Too much players connected. Try again later.");
                    }

                    newPlayer.reset();
                    newPlayer.name = name;
                    newPlayer.IP = rAddr.toString();
                    newPlayer.port = rPort;
                    newPlayer.connected = true;
                }
            }
            else
            {
                logMessage("UDP: Sesskey rejected");
                Player newPlayer;
                newPlayer.IP = rAddr.toString();
                newPlayer.port = rPort;
                sendMessage(newPlayer, MsgDisconnect, "Error : Wrong sesskey hash.");
                return;
            }
        }

        Player& player = Player::findPlayer(udpPlayers, rAddr.toString(), rPort);
        if (player.IP == rAddr.toString() && player.port == rPort)
        {
            // Acquire datas
            player.receivedDatas->append(datagram);

            // Process data
            receiveMessage(player);
        }
        else // You need to connect with TCP first
        {
            logMessage("UDP: Request from unknow peer rejected : "+player.IP+":"+QString().setNum(rPort));
            sendMessage(player,MsgDisconnect);
        }
    }
}
