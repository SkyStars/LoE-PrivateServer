#include "widget.h"
#include "message.h"
#include "utils.h"
#include "sync.h"

#include <sys/time.h>

float timestampNow()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (float)(((float)(tp.tv_nsec - win.startTimestamp))/(float)1000); // Nombre de seconde depuis le lancement du serveur (startTimestamp)
}

QByteArray doubleToData(double num)
{
    union
    {
        char tab[8];
        double n;
    } castUnion;
    //char n[8];
    //*((double*) n) = num;

    castUnion.n=num;
    return QByteArray(castUnion.tab,8);
}

QByteArray floatToData(float num)
{
    union
    {
        char tab[4];
        float n;
    } castUnion;

    castUnion.n=num;
    return QByteArray(castUnion.tab,4);
}

float dataToFloat(QByteArray data)
{
    union
    {
        char tab[4];
        float n;
    } castUnion;

    castUnion.tab[0]=data.data()[0];
    castUnion.tab[1]=data.data()[1];
    castUnion.tab[2]=data.data()[2];
    castUnion.tab[3]=data.data()[3];
    return castUnion.n;
}

// Converts a string into PNet string data
QByteArray stringToData(QString str)
{
    QByteArray data(4,0);
    // Write the size in a Uint of variable lenght (8-32 bits)
    int i=0;
    uint num1 = (uint)str.size();
    while (num1 >= 0x80)
    {
        data[i] = (unsigned char)(num1 | 0x80); i++;
        num1 = num1 >> 7;
    }
    data[i]=num1;
    data.resize(i+1);
    data+=str;
    return data;
}

QString dataToString(QByteArray data)
{
    // Variable UInt32
    unsigned char num3;
    int num = 0;
    int num2 = 0;
    int i=0;
    do
    {
        num3 = data[i]; i++;
        num |= (num3 & 0x7f) << num2;
        num2 += 7;
    } while ((num3 & 0x80) != 0);
    unsigned int strlen = (uint) num;

    if (!strlen)
        return QString();

    data = data.right(data.size()-i); // Remove the strlen
    data.truncate(strlen);
    return QString(data);
}

QByteArray vectorToData(UVector vec)
{
    QByteArray data;
    data += floatToData(vec.x);
    data += floatToData(vec.y);
    data += floatToData(vec.z);
    return data;
}

UVector dataToVector(QByteArray data)
{
    UVector vec;
    vec.x = dataToFloat(data);
    vec.y = dataToFloat(data.mid(4));
    vec.z = dataToFloat(data.mid(8));
    return vec;
}

QByteArray quaternionToData(UQuaternion quat)
{
    QByteArray data;
    data += floatToData(quat.x);
    data += floatToData(quat.y);
    data += floatToData(quat.z);
    data += floatToData(quat.w);
    return data;
}

float dataToRangedSingle(float min, float max, int numberOfBits, QByteArray data)
{
    /* READ UINT 32
    public uint ReadUInt32(int numberOfBits)
    {
        uint num = NetBitWriter.ReadUInt32(this.m_data, numberOfBits, this.m_readPosition);
        this.m_readPosition += numberOfBits;
        return num;
    }

    // NETBITWRITER READ UINT 32
public static uint ReadUInt32(unsigned char[] fromBuffer, int numberOfBits, int readBitOffset)
{
    if (numberOfBits <= 8)
    {
        return ReadByte(fromBuffer, numberOfBits, readBitOffset);
    }
    uint num = ReadByte(fromBuffer, 8, readBitOffset);
    numberOfBits -= 8;
    readBitOffset += 8;
    if (numberOfBits <= 8)
    {
        return (num | ((uint) (ReadByte(fromBuffer, numberOfBits, readBitOffset) << 8)));
    }
    num |= (uint) (ReadByte(fromBuffer, 8, readBitOffset) << 8);
    numberOfBits -= 8;
    readBitOffset += 8;
    if (numberOfBits <= 8)
    {
        uint num2 = (uint) (ReadByte(fromBuffer, numberOfBits, readBitOffset) << 0x10);
        return (num | num2);
    }
    num |= (uint) (ReadByte(fromBuffer, 8, readBitOffset) << 0x10);
    numberOfBits -= 8;
    readBitOffset += 8;
    return (num | ((uint) (ReadByte(fromBuffer, numberOfBits, readBitOffset) << 0x18)));
}
    */

    uint endvalue=0;
    uint value=0;
    if (numberOfBits <= 8)
    {
        endvalue = (uchar)data[0];
        goto done;
    }
    value = (uchar)data[0];
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        endvalue = (value | ((uint) ((uchar)data[1]) << 8));
        goto done;
    }
    value |= (uint) (((uchar)data[1]) << 8);
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        uint num2 = (uint) (((uchar)data[2]) << 0x10);
        endvalue = (value | num2);
        goto done;
    }
    value |= (uint) (((uchar)data[2]) << 0x10);
    numberOfBits -= 8;
    endvalue =  (value | ((uint) (((uchar)data[3]) << 0x18)));
    goto done;

    done:

    float num = max - min;
    int num2 = (((int) 1) << numberOfBits) - 1;
    float num3 = endvalue;
    float num4 = num3 / ((float) num2);
    return (min + (num4 * num));
}

QByteArray rangedSingleToData(float value, float min, float max, int numberOfBits)
{
    QByteArray data;
    float num = max - min;
    float num2 = (value - min) / num;
    int num3 = (((int) 1) << numberOfBits) - 1;
    uint source = num3 * num2;

    if (numberOfBits <= 8)
    {
        data += (unsigned char)source;
        return data;
    }
    data += (unsigned char)source;
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        data += (unsigned char)source>>8;
        return data;
    }
    data += (unsigned char)source>>8;
    numberOfBits -= 8;
    if (numberOfBits <= 8)
    {
        data += (unsigned char)source>>16;
        return data;
    }
    data += (unsigned char)source>>16;
    numberOfBits -= 8;
    data += (unsigned char)source>>24;

    return data;
}

void receiveMessage(Player& player)
{
    QByteArray msg = *(player.receivedDatas);
    int msgSize=5 + (((unsigned char)msg[3]) + (((unsigned char)msg[4]) << 8))/8;

    // Check seq
    if ((unsigned char)msg[0] >= MsgUserReliableOrdered1 && (unsigned char)msg[0] <= MsgUserReliableOrdered32)
    {
        quint16 seq = msg[1] + (msg[2]<<8);
        quint8 channel = ((unsigned char)msg[0])-MsgUserReliableOrdered1;
        if (seq < player.udpRecvSequenceNumbers[channel])
        {
            win.logMessage("UDP: Discarding double message from "+player.IP+":"+QString().setNum(player.port));
            *player.receivedDatas = player.receivedDatas->right(player.receivedDatas->size() - msgSize);
            //if (player.receivedDatas->size())
            //    receiveMessage(player);
            return; // We already have this packet.
        }
        else if (seq > player.udpRecvSequenceNumbers[channel]+1)
        {
            win.logMessage("UDP: Unordered message received from "+QString().setNum(player.pony.netviewId));
        }
        else
            player.udpRecvSequenceNumbers[channel] = seq;
    }

    if ((unsigned char)msg[0] == MsgPing) // Ping
    {
        win.logMessage("UDP: Ping received from "+player.IP+":"+QString().setNum(player.port)
                +" ("+QString().setNum((timestampNow() - player.lastPingTime))+"s)");
        player.lastPingNumber = msg[5];
        player.lastPingTime = timestampNow();
        sendMessage(player,MsgPong);
    }
    else if ((unsigned char)msg[0] == MsgPong) // Pong
    {
        win.logMessage("UDP: Pong received");
    }
    else if ((unsigned char)msg[0] == MsgConnect) // Connect SYN
    {
        msg.resize(18); // Supprime le message LocalHail et le Timestamp
        msg = msg.right(13); // Supprime le Header

        win.logMessage(QString("UDP: Connecting ..."));

        Player &refresh = Player::findPlayer(win.udpPlayers, player.IP, player.port);
        if (refresh.IP == "")
        {
            win.logMessage("Can't refresh player at connection, aborting");
            return;
        }

        for (int i=0; i<32; i++) // Reset sequence counters
            refresh.udpSequenceNumbers[i]=0;

        sendMessage(refresh, MsgConnectResponse, msg);
    }
    else if ((unsigned char)msg[0] == MsgConnectionEstablished) // Connect ACK
    {
        win.logMessage("UDP: Connected to client");
        player.connected=true;
        for (int i=0; i<32; i++) // Reset sequence counters
            player.udpSequenceNumbers[i]=0;

        // Start game
        win.logMessage(QString("UDP: Starting game"));

        // Set local player id
        win.lastId++;
        win.lastNetviewId++;
        player.pony.id = win.lastId;
        player.pony.netviewId = win.lastNetviewId;

        win.logMessage("UDP: Set id request : " + QString().setNum(player.pony.id) + "/" + QString().setNum(player.pony.netviewId));

        // Set player Id request
        QByteArray id(3,0);
        id[0]=4;
        id[1]=player.pony.id;
        id[2]=player.pony.id>>8;
        sendMessage(player,MsgUserReliableOrdered6,id); // Sends a 48

        // Load Characters screen requets
        QByteArray data(1,5);
        data += stringToData("Characters");
        sendMessage(player,MsgUserReliableOrdered6,data); // Sends a 48
    }
    else if ((unsigned char)msg[0] == MsgAcknowledge) // Acknoledge
    {
        int nAcks = (msg[3] + (msg[4]<<8)) / 24;
        if (nAcks)
        {
            QString ackMsg = "UDP: Messages acknoledged (";
            for (int i=0; i<nAcks; i++)
            {
                if (i)
                    ackMsg += "/";
                ackMsg+=msg.mid(3*i+5,3).toHex();
            }
            ackMsg += ")";
            //win.logMessage(ackMsg);
        }
    }
    else if ((unsigned char)msg[0] == MsgDisconnect) // Disconnect
    {
        win.logMessage("UDP: Client disconnected");
        Player::disconnectPlayerCleanup(player); // Save game and remove the player

        return; // We can't use Player& player anymore, it referes to free'd memory.
    }
    else if ((unsigned char)msg[0] >= MsgUserReliableOrdered1 && (unsigned char)msg[0] <= MsgUserReliableOrdered32) // UserReliableOrdered
    {
        //win.logMessage("UDP: Data received (hex) : ");
        //win.logMessage(player.receivedDatas->toHex().constData());

        QByteArray data(3,0);
        data[0] = msg[0]; // ack type
        data[1] = msg[1]/2; // seq
        data[2] = msg[2]/2; // seq
        sendMessage(player, MsgAcknowledge, data);

        if ((unsigned char)msg[0]==MsgUserReliableOrdered6 && (unsigned char)msg[3]==8 && (unsigned char)msg[4]==0 && (unsigned char)msg[5]==6 ) // Prefab (player/mobs) list instantiate request
        {
            sendEntitiesList(player);
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered6 && (unsigned char)msg[3]==0x18 && (unsigned char)msg[4]==0 && (unsigned char)msg[5]==8 ) // Player game info (inv/ponyData/...) request
        {
            sendPonySave(player, msg);
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered4 && (unsigned char)msg[5]==0x1) // Edit ponies request
        {
            QList<Pony> ponies = Player::loadPonies(player);
            QByteArray ponyData = msg.right(msg.size()-10);
            Pony pony;
            if ((unsigned char)msg[6]==0xff && (unsigned char)msg[7]==0xff && (unsigned char)msg[8]==0xff && (unsigned char)msg[9]==0xff)
            {
                // Create the new pony for this player
                pony.ponyData = ponyData;
                pony.sceneName = "PonyVille";
                pony.pos = findVortex(pony.sceneName, 0)->destPos;
                ponies += pony;
            }
            else
            {
                quint32 id = msg[6] +(msg[7]<<8) + (msg[8]<<16) + (msg[9]<<24);
                ponies[id].ponyData = ponyData;
                pony = ponies[id];
            }
            pony.id = player.pony.id;
            pony.netviewId = player.pony.netviewId;
            player.pony = pony;

            Player::savePonies(player, ponies);

            sendLoadSceneRPC(player, player.pony.sceneName, player.pony.pos);
            // Send instantiate to the players of the new scene
            Scene* scene = findScene(player.pony.sceneName);
            for (int i=0; i<scene->players.size(); i++)
                if (scene->players[i].pony.netviewId!=player.pony.netviewId && scene->players[i].inGame==2)
                    sendNetviewInstantiate(player, scene->players[i]);

            //Send the 46s init messages
            win.logMessage(QString("UDP: Sending the 46 init messages"));
            sendMessage(player,MsgUserReliableOrdered4,QByteArray::fromHex("141500000000")); // Sends a 46, init friends
            sendMessage(player,MsgUserReliableOrdered4,QByteArray::fromHex("0e00000000")); // Sends a 46, init journal
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered20 && (unsigned char)msg[3]==0x18 && (unsigned char)msg[4]==0) // Vortex messages
        {
            if (player.inGame==2)
            {
                quint8 id = msg[5];
                Vortex* vortex = findVortex(player.pony.sceneName, id);
                if (vortex->destName.isEmpty())
                {
                    win.logMessage("Can't find vortex "+QString().setNum(id)+" on map "+player.pony.sceneName);
                }
                else
                {
                    sendLoadSceneRPC(player, vortex->destName, vortex->destPos);
                }
            }
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered4 && (unsigned char)msg[5]==0x2) // Delete pony request
        {
            win.logMessage(QString("UDP: Deleting a character"));
            QList<Pony> ponies = Player::loadPonies(player);
            quint32 id = msg[6] +(msg[7]<<8) + (msg[8]<<16) + (msg[9]<<24);
            ponies.removeAt(id);

            Player::savePonies(player,ponies);
            sendPonies(player);
        }
        else
        {
            // Display data
            win.logMessage("UDP: Unknow data received : "+QString(player.receivedDatas->toHex().data()));
            player.receivedDatas->clear();
            msgSize=0;
        }
    }
    else if ((unsigned char)msg[0]==MsgUserUnreliable) // Sync (position) update
    {
        if ((unsigned char)msg[5]==(unsigned char)player.pony.netviewId && (unsigned char)msg[6]==(unsigned char)player.pony.netviewId>>8)
            receiveSync(player, msg);
    }
    else
    {
        // Display data
        win.logMessage("Data received (UDP) (hex) : ");
        win.logMessage(QString(player.receivedDatas->toHex().data()));
        player.receivedDatas->clear();
        msgSize=0;
    }

    *player.receivedDatas = player.receivedDatas->right(player.receivedDatas->size() - msgSize);
    if (player.receivedDatas->size())
        receiveMessage(player);
}

void sendMessage(Player& player,quint8 messageType, QByteArray data)
{
    Player &refresh = Player::findPlayer(win.udpPlayers, player.IP, player.port);
    if (refresh.IP == "")
    {
        win.logMessage("Can't refresh player before sendMessage, aborting");
        return;
    }
    player = refresh;

    QByteArray msg(3,0);
    // MessageType
    msg[0] = messageType;
    // Sequence
    msg[1] = 0;
    msg[2] = 0;
    if (messageType == MsgPing)
    {
        msg.resize(6);
        // Sequence
        msg[1] = 0;
        msg[2] = 0;
        // Payload size
        msg[3] = 8;
        msg[4] = 0;
        // Ping number
        refresh.lastPingNumber++;
        msg[5]=refresh.lastPingNumber;
    }
    else if (messageType == MsgPong)
    {
        msg.resize(6);
        // Payload size
        msg[3] = 8*5;
        msg[4] = 0;
        // Ping number
        msg[5]=refresh.lastPingNumber;
        // Timestamp
        msg += floatToData(timestampNow());
    }
    else if (messageType == MsgUserUnreliable)
    {
        msg.resize(5);
        // Sequence
        msg[1] = refresh.udpSequenceNumbers[32];
        msg[2] = refresh.udpSequenceNumbers[32]>>8;
        // Payload size
        msg[3] = 8*(data.size());
        msg[4] = (8*(data.size())) >> 8;
        // Timestamp
        msg += data;
        refresh.udpSequenceNumbers[32]++;

        //win.logMessage(QString("Sending sync data :")+msg.toHex());
    }
    else if (messageType >= MsgUserReliableOrdered1 && messageType <= MsgUserReliableOrdered32)
    {
        msg.resize(5);
        // Sequence
        msg[1] = refresh.udpSequenceNumbers[messageType-MsgUserReliableOrdered1];
        msg[2] = refresh.udpSequenceNumbers[messageType-MsgUserReliableOrdered1] >> 8;
        // Payload size
        msg[3] = 8*(data.size());
        msg[4] = (8*(data.size())) >> 8;
        // strlen
        msg+=data;
        refresh.udpSequenceNumbers[messageType-MsgUserReliableOrdered1] += 2;
        //win.logMessage(QString("Sending message ")+msg.mid(0,3).toHex());
        //win.logMessage(QString("Sending data :")+msg.toHex());
        //win.logMessage("Sending msg with seq " + QString().setNum(refresh.udpSequenceNumbers[messageType-MsgUserReliableOrdered1]));
    }
    else if (messageType == MsgAcknowledge)
    {
        msg.resize(5);
        // Payload size
        msg[3] = data.size()*8;
        msg[4] = 0;
        msg.append(data); // Format of packet data n*(Ack type, Ack seq, Ack seq)
    }
    else if (messageType == MsgConnectResponse)
    {
        msg.resize(5);
        // Payload size
        msg[3] = 0x88;
        msg[4] = 0x00;

        //win.logMessage("UDP: Header data : "+msg.toHex());

        // AppId + UniqueId
        msg += data;
        msg += floatToData(timestampNow());
    }
    else if (messageType == MsgDisconnect)
    {
        msg.resize(6);
        // Payload size
        msg[3] = ((data.size()+1)*8);
        msg[4] = ((data.size()+1)*8)>>8;
        // Message length
        msg[5] = data.size();
        // Disconnect message
        msg += data;
    }
    else
    {
        win.logStatusMessage("SendMessage : Unknow message type");
        return;
    }
    if (win.udpSocket->writeDatagram(msg,QHostAddress(player.IP),player.port) != msg.size())
    {
        win.logMessage("UDP: Error sending last message");
        win.logStatusMessage("Restarting UDP server ...");
        win.udpSocket->close();
        if (!win.udpSocket->bind(UDPPORT, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress))
        {
            win.logStatusMessage("UDP: Unable to start server on port 80");
            win.stopServer();
            return;
        }
    }
}

void sendPonies(Player& player)
{
    // The full request is like a normal sendPonies but with all the serialized ponies at the end
    QList<Pony> ponies = Player::loadPonies(player);
    quint32 poniesDataSize=0;
    for (int i=0;i<ponies.size();i++)
        poniesDataSize+=ponies[i].ponyData.size();

    QByteArray data(5,0);
    data[0] = 1; // Request number
    data[1] = ponies.size(); // Number of ponies
    data[2] = ponies.size()>>8; // Number of ponies
    data[3] = ponies.size()>>16; // Number of ponies
    data[4] = ponies.size()>>24; // Number of ponies

    for (int i=0;i<ponies.size();i++)
        data += ponies[i].ponyData;

    win.logMessage(QString("UDP: Sending characters data to ")+QString().setNum(player.pony.netviewId));
    sendMessage(player, MsgUserReliableOrdered4, data);
}

void sendEntitiesList(Player& player)
{
    if (!player.inGame) // Not yet in game, send player's ponies list
    {
        sendPonies(player);
        //player.inGame=1;
        return;
    }

    // Loading finished, sending entities list
    Player &refresh = Player::findPlayer(win.udpPlayers, player.IP, player.port);
    win.logMessage(QString("UDP: Sending entities list to ")+QString().setNum(refresh.pony.netviewId));
    if (refresh.IP == "")
    {
        win.logMessage("Can't refresh player to remove loading status, aborting");
        return;
    }
    if (refresh.inGame!=1) // Only instantiate once
        return;

    Scene* scene = findScene(refresh.pony.sceneName);
    for (int i=0; i<scene->players.size(); i++)
        sendNetviewInstantiate(scene->players[i], refresh);
    refresh.inGame = 2;
}

void sendPonySave(Player& player, QByteArray msg)
{
    quint16 netviewId = msg[6] + (msg[7]<<8);
    Player& refresh = Player::findPlayer(win.udpPlayers, netviewId);

    if (netviewId == player.pony.netviewId)
    {
        // Send the entities list if the game is starting (sends RPC calls to the view with id 0085 (the PlayerBase))
        win.logMessage(QString("UDP: Sending pony save to ")+QString().setNum(netviewId));

        // Set current/max stats
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033000000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032000000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033010000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032010000c842")); // Sends a 54
        sendSetMaxStatRPC(player, 0, 100);
        sendSetStatRPC(player, 0, 100);
        sendSetMaxStatRPC(player, 1, 100);
        sendSetStatRPC(player, 1, 100);

        //Scene* scene = findScene(player.pony.sceneName);
        //for (int i=0; i<scene->players.size(); i++)
        sendPonyData(player);

        // Set inventory (always empty at the moment)
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("8500050c0020000a000000")); // Sends a 54
        QList<InventoryItem> inv;
        QList<WearableItem> worn;
        sendInventoryRPC(player, inv, worn, 10); // Start with 10 bits and no inventory, until we implement it correctly

        // Send skills (always the same at the moment)
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("8500c30300000000000000ffffff7f01000000ffffff7f05000000ffffff7f")); // Sends a 54
        QList<QPair<quint32, quint32> > skills;
        skills << QPair<quint32, quint32>(0, 0x7FFFFFFF);
        skills << QPair<quint32, quint32>(1, 0x7FFFFFFF);
        skills << QPair<quint32, quint32>(5, 0x7FFFFFFF);
        sendSkillsRPC(player, skills);

        // Set current/max stats
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032000000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033000000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032010000c842")); // Sends a 54
        //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033010000c842")); // Sends a 54
        sendSetStatRPC(player, 0, 100);
        sendSetMaxStatRPC(player, 0, 100);
        sendSetStatRPC(player, 1, 100);
        sendSetMaxStatRPC(player, 1, 100);
    }
    else if (!refresh.IP.isEmpty())
    {
        win.logMessage(QString("UDP: Sending pony save for ")+QString().setNum(refresh.pony.netviewId)+" to "+QString().setNum(player.pony.netviewId));
        sendPonyData(refresh, player);
    }
    else
    {
        win.logStatusMessage("UDP: Error sending pony save : netviewId not found");
    }
}

void receiveSync(Player& player, QByteArray data) // Receives the 01 updates from each players
{
    if (player.inGame!=2) // A sync message while loading would teleport use to a wrong position
        return;

    // 5 and 6 are id and id>>8
    player.pony.pos.x = dataToFloat(data.right(data.size()  - 11));
    player.pony.pos.y = dataToFloat(data.right(data.size()  - 15));
    player.pony.pos.z = dataToFloat(data.right(data.size()  - 19));

    // TODO : Get the rotation and save it
    if (data.size() >= 23)
        player.pony.rot.y = dataToRangedSingle(ROTMIN, ROTMAX, RotRSSize, data.mid(23,1));
    if (data.size() >= 25)
    {
        player.pony.rot.x = dataToRangedSingle(ROTMIN, ROTMAX, RotRSSize, data.mid(24,1));
        player.pony.rot.z = dataToRangedSingle(ROTMIN, ROTMAX, RotRSSize, data.mid(25,1));
    }
}

void sendNetviewInstantiate(Player& player, QString key, quint16 ViewId, quint16 OwnerId, UVector pos, UQuaternion rot)
{
    QByteArray data(1,1);
    data += stringToData(key);
    QByteArray data2(4,0);
    data2[0]=ViewId;
    data2[1]=ViewId>>8;
    data2[2]=OwnerId;
    data2[3]=OwnerId>>8;
    data += data2;
    data += vectorToData(pos);
    data += quaternionToData(rot);
    sendMessage(player, MsgUserReliableOrdered6, data);
}

void sendNetviewInstantiate(Player& player)
{
    win.logMessage("UDP: Send instantiate for/to "+QString().setNum(player.pony.netviewId));
    QByteArray data(1,1);
    data += stringToData("PlayerBase");
    QByteArray data2(4,0);
    data2[0]=player.pony.netviewId;
    data2[1]=player.pony.netviewId>>8;
    data2[2]=player.pony.id;
    data2[3]=player.pony.id>>8;
    data += data2;
    data += vectorToData(player.pony.pos);
    data += quaternionToData(player.pony.rot);
    sendMessage(player, MsgUserReliableOrdered6, data);
}

void sendNetviewInstantiate(Player& src, Player& dst)
{
    win.logMessage("UDP: Send instantiate for "+QString().setNum(src.pony.netviewId)+" to "+QString().setNum(dst.pony.netviewId));
    QByteArray data(1,1);
    data += stringToData("PlayerBase");
    QByteArray data2(4,0);
    data2[0]=src.pony.netviewId;
    data2[1]=src.pony.netviewId>>8;
    data2[2]=src.pony.id;
    data2[3]=src.pony.id>>8;
    data += data2;
    data += vectorToData(src.pony.pos);
    data += quaternionToData(src.pony.rot);
    sendMessage(dst, MsgUserReliableOrdered6, data);
}

void sendNetviewRemove(Player& player, quint16 netviewId)
{
    win.logMessage("UDP: Removing netview "+QString().setNum(netviewId)+" to "+QString().setNum(player.pony.netviewId));

    QByteArray data(3,2);
    data[1] = netviewId;
    data[2] = netviewId>>8;
    sendMessage(player, MsgUserReliableOrdered6, data);
}

void sendSetStatRPC(Player& player, quint8 statId, float value)
{
    QByteArray data(4,50);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data[3] = statId;
    data += floatToData(value);
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendSetMaxStatRPC(Player& player, quint8 statId, float value)
{
    QByteArray data(4,51);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data[3] = statId;
    data += floatToData(value);
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendInventoryRPC(Player& player, QList<InventoryItem> inv, QList<WearableItem> worn, quint32 nBits)
{
    QByteArray data(5, 5);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data[3] = 12; // Max Inventory Size
    data[4] = inv.size();
    for (int i=0;i<inv.size();i++)
    {
        data += inv[i].index;
        data += inv[i].id;
        data += inv[i].id>>8;
        data += inv[i].id>>16;
        data += inv[i].id>>24;
        data += inv[i].amount;
        data += inv[i].amount>>8;
        data += inv[i].amount>>16;
        data += inv[i].amount>>24;
    }
    data += 32; // Max Worn Items
    data += worn.size();
    for (int i=0;i<worn.size();i++)
    {
        data += worn[i].index;
        data += worn[i].id;
        data += worn[i].id>>8;
        data += worn[i].id>>16;
        data += worn[i].id>>24;
    }
    data += nBits;
    data += nBits>>8;
    data += nBits>>16;
    data += nBits>>24;
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendSkillsRPC(Player& player, QList<QPair<quint32, quint32> > skills)
{
    QByteArray data(7, 0xC3);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data[3] = skills.size();
    data[4] = skills.size()>>8;
    data[5] = skills.size()>>16;
    data[6] = skills.size()>>24;
    for (int i=0;i<skills.size();i++)
    {
        data += skills[i].first;
        data += skills[i].first>>8;
        data += skills[i].first>>16;
        data += skills[i].first>>24;
        data += skills[i].second;
        data += skills[i].second>>8;
        data += skills[i].second>>16;
        data += skills[i].second>>24;
    }
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendPonyData(Player& player)
{
    // Sends the ponyData
    win.logMessage(QString("UDP: Sending the ponyData for/to "+QString().setNum(player.pony.netviewId)));
    QByteArray data(3,0xC8);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data += player.pony.ponyData;
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendPonyData(Player& src, Player& dst)
{
    // Sends the ponyData
    win.logMessage(QString("UDP: Sending the ponyData for "+QString().setNum(src.pony.netviewId)+" to "+QString().setNum(dst.pony.netviewId)));
    QByteArray data(3,0xC8);
    data[0] = src.pony.netviewId;
    data[1] = src.pony.netviewId>>8;
    data += src.pony.ponyData;
    sendMessage(dst, MsgUserReliableOrdered18, data);
}

void sendLoadSceneRPC(Player &player, QString sceneName) // Loads a scene and send to the default spawn
{
    win.logMessage(QString("UDP: Loading scene \"") + sceneName + "\" to "+QString().setNum(player.pony.netviewId));
    Vortex* vortex = findVortex(sceneName, 0);
    if (vortex->destName.isEmpty())
    {
        win.logMessage("UDP: Scene not in vortex DB. Aborting scene load.");
        return;
    }

    Player &refresh = Player::findPlayer(win.udpPlayers, player.IP, player.port);
    if (refresh.IP == "")
    {
        win.logMessage("UDP: Can't refresh player before loading scene, aborting");
        return;
    }
    refresh.inGame=1;

    Scene* scene = findScene(sceneName);
    Scene* oldScene = findScene(refresh.pony.sceneName);
    if (scene->name.isEmpty() || oldScene->name.isEmpty())
    {
        win.logMessage("UDP: Can't find the scene, aborting");
        return;
    }

    // Update scene players
    Player::removePlayer(&(oldScene->players), refresh.IP, refresh.port);
    if (scene->name != sceneName)
    {
        // Send remove RPC to the other players of the old scene
        for (int i=0; i<oldScene->players.size(); i++)
            sendNetviewRemove(oldScene->players[i], refresh.pony.netviewId);

        // Send instantiate to the players of the new scene
        for (int i=0; i<scene->players.size(); i++)
            if (scene->players[i].inGame==2)
                sendNetviewInstantiate(refresh, scene->players[i]);
    }
    scene->players << refresh;

    refresh.pony.pos = vortex->destPos;
    refresh.pony.sceneName = sceneName;
    QByteArray data(1,5);
    data += stringToData(sceneName);
    sendMessage(refresh,MsgUserReliableOrdered6,data); // Sends a 48
}

void sendLoadSceneRPC(Player &player, QString sceneName, UVector pos) // Loads a scene and send to the given pos
{
    win.logMessage(QString(QString("UDP: Loading scene \"") + sceneName + "\" to "+QString().setNum(player.pony.netviewId)));
    Player &refresh = Player::findPlayer(win.udpPlayers, player.IP, player.port);
    if (refresh.IP == "")
    {
        win.logMessage("UDP: Can't refresh player before loading scene, aborting");
        return;
    }

    refresh.inGame=1;

    Scene* scene = findScene(sceneName);
    Scene* oldScene = findScene(refresh.pony.sceneName);
    if (scene->name.isEmpty() || oldScene->name.isEmpty())
    {
        win.logMessage("UDP: Can't find the scene, aborting");
        return;
    }

    // Update scene players
    Player::removePlayer(&(oldScene->players), refresh.IP, refresh.port);
    if (scene->name != sceneName)
    {
        // Send remove RPC to the other players of the old scene
        for (int i=0; i<oldScene->players.size(); i++)
            sendNetviewRemove(oldScene->players[i], refresh.pony.netviewId);

        // Send instantiate to the players of the new scene
        for (int i=0; i<scene->players.size(); i++)
            if (scene->players[i].inGame==2)
                sendNetviewInstantiate(refresh, scene->players[i]);
    }
    scene->players << refresh;

    refresh.pony.pos = pos;
    refresh.pony.sceneName = sceneName;
    QByteArray data(1,5);
    data += stringToData(sceneName);
    sendMessage(refresh,MsgUserReliableOrdered6,data); // Sends a 48
}
