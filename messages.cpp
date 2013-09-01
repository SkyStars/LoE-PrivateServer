#include "widget.h"
#include "message.h"
#include "utils.h"

float timestampNow()
{
    return (float)(((float)(GetTickCount() - win.startTimestamp))/(float)1000); // Nombre de seconde depuis le lancement du serveur (startTimestamp)
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
        data[i] = (byte)(num1 | 0x80); i++;
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
    byte num3;
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

void receiveMessage(Player& player)
{
    QByteArray msg = *(player.receivedDatas);
    int msgSize=5 + (((unsigned char)msg[3]) + (((unsigned char)msg[4]) << 8))/8;
    if ((unsigned char)msg[0] == MsgPing) // Ping
    {
        //win.logMessage("UDP : Ping received from "+player.IP+":"+QString().setNum(player.port));
        //win.logMessage("UDP : Last ping Dtime : "+QString().setNum((int)(timestampNow() - player.lastPingTime)));
        player.lastPingNumber = msg[5];
        player.lastPingTime = timestampNow();
        sendMessage(player,MsgPong);
    }
    else if ((unsigned char)msg[0] == MsgPong) // Pong
    {
        win.logMessage("UDP : Pong received");
    }
    else if ((unsigned char)msg[0] == MsgConnect) // Connect SYN
    {
        msg.resize(18); // Supprime le message LocalHail et le Timestamp
        msg = msg.right(13); // Supprime le Header

        win.logMessage(QString("UDP : Connecting ..."));

        for (int i=0; i<32; i++) // Reset sequence counters
            player.udpSequenceNumbers[i]=0;

        sendMessage(player, MsgConnectResponse, msg);
    }
    else if ((unsigned char)msg[0] == MsgConnectionEstablished) // Connect ACK
    {
        win.logMessage("UDP : Connected to client");
        player.connected=true;
        for (int i=0; i<32; i++) // Reset sequence counters
            player.udpSequenceNumbers[i]=0;

        // Start game
        win.logMessage(QString("UDP : Starting game"));

        // Set local player id
        // TODO: Use a unique id per player starting from 1
        player.pony.id = 0x64;
        player.pony.netviewId = 0x85;

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
        win.logMessage("UDP : Message acknoledged");
    }
    else if ((unsigned char)msg[0] == MsgDisconnect) // Disconnect
    {
        win.logMessage("UDP : Client disconnected");
        Player::disconnectPlayerCleanup(player); // Save game and remove the player

        return; // We can't use Player& player anymore, it referes to free'd memory.
    }
    else if ((unsigned char)msg[0] >= MsgUserReliableOrdered1 && (unsigned char)msg[0] <= MsgUserReliableOrdered32) // UserReliableOrdered
    {
        win.logMessage("UDP : Data received (hex) : ");
        win.logMessage(player.receivedDatas->toHex().constData());

        QByteArray data(3,0);
        data[0] = msg[0]; // ack type
        data[1] = msg[1]/2; // seq
        data[2] = msg[2]/2; // seq
        sendMessage(player, MsgAcknowledge, data);

        if ((unsigned char)msg[0]==MsgUserReliableOrdered6 && (unsigned char)msg[3]==8 && (unsigned char)msg[4]==0 && (unsigned char)msg[5]==6 ) // Request player/mobs (entities) request
        {
            sendEntitiesList(player);
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered6 && (unsigned char)msg[3]==0x18 && (unsigned char)msg[4]==0 && (unsigned char)msg[5]==8 ) // Request player/mobs (entities) request for reply 54
        {
            sendEntitiesList2(player);
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
                pony.sceneName = "Ponyville";
                pony.pos = UVector(-3.80378, -195.301, 176.69); // In front of the Town Hall
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

            win.logMessage(QString("UDP : Requesting scene"));
            QByteArray data(1,5);
            data += stringToData(player.pony.sceneName);
            sendMessage(player,MsgUserReliableOrdered6,data); // Sends a 48

            //Send the 46s
            win.logMessage(QString("UDP : Sending the 46 messages"));

            sendMessage(player,MsgUserReliableOrdered4,QByteArray::fromHex("141500000000")); // Sends a 46, init friends
            sendMessage(player,MsgUserReliableOrdered4,QByteArray::fromHex("0e00000000")); // Sends a 46, init journal
        }
        else if ((unsigned char)msg[0]==MsgUserReliableOrdered4 && (unsigned char)msg[5]==0x2) // Delete pony request
        {
            win.logMessage(QString("UDP : Deleting a character"));
            QList<Pony> ponies = Player::loadPonies(player);
            quint32 id = msg[6] +(msg[7]<<8) + (msg[8]<<16) + (msg[9]<<24);
            ponies.removeAt(id);

            Player::savePonies(player,ponies);
            sendPonies(player);
        }
        else
        {
            // Display data
            win.logMessage("Data received (UDP) (hex) : ");
            win.logMessage(QString(player.receivedDatas->toHex().data()));
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
        player.lastPingNumber++;
        msg[5]=player.lastPingNumber;
    }
    else if (messageType == MsgPong)
    {
        msg.resize(6);
        // Payload size
        msg[3] = 8*5;
        msg[4] = 0;
        // Ping number
        msg[5]=player.lastPingNumber;
        // Timestamp
        msg += floatToData(timestampNow());
    }
    else if (messageType >= MsgUserReliableOrdered1 && messageType <= MsgUserReliableOrdered32)
    {
        msg.resize(5);
        // Sequence
        msg[1] = player.udpSequenceNumbers[messageType-MsgUserReliableOrdered1];
        msg[2] = player.udpSequenceNumbers[messageType-MsgUserReliableOrdered1] >> 8;
        // Payload size
        msg[3] = 8*(data.size());
        msg[4] = (8*(data.size())) >> 8;
        // strlen
        msg+=data;
        player.udpSequenceNumbers[messageType-MsgUserReliableOrdered1] += 2;
        //win.logMessage(QString("Sending data :")+msg.toHex());
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

        //win.logMessage("UDP : Header data : "+msg.toHex());

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
        win.logMessage("UDP : Error sending last message");
        win.logStatusMessage("Restarting UDP server ...");
        win.udpSocket->close();
        if (!win.udpSocket->bind(UDPPORT, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress))
        {
            win.logStatusMessage("UDP : Unable to start server on port 80");
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

    win.logMessage(QString("UDP : Sending characters data"));
    sendMessage(player, MsgUserReliableOrdered4, data);
}

void sendEntitiesList(Player& player)
{
    if (!player.inGame)
    {
        sendPonies(player);
        player.inGame=true;
        return;
    }

    // Send the entities list if a new scene is loading
    win.logMessage(QString("UDP : Sending entities list"));
    sendNetviewInstantiate(player);
    //sendMessage(player, MsgUserReliableOrdered6, QByteArray::fromHex("010a506c6179657242617365850064007f45fd41be2195bf3e0e6ac200000000fd987ebf000000000f13d63d"));

    // Sends the ponyData
    //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("8500c80a4d7564204f72616e676503034b000000000000000000000080c6ff9fd4fff2ff80df80ff85bf3401000900010000002e55693fdf69")); // Sends a 54
    QByteArray data(3,0xC8);
    data[0] = player.pony.netviewId;
    data[1] = player.pony.netviewId>>8;
    data += player.pony.ponyData;
    sendMessage(player, MsgUserReliableOrdered18, data);
}

void sendEntitiesList2(Player& player)
{
    // Send the entities list if the game is starting (sends RPC calls to the view with id 0085 (the PlayerBase))
    win.logMessage(QString("UDP : Sending entities list 2"));

    // Set current/max stats
    //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033000000c842")); // Sends a 54
    //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032000000c842")); // Sends a 54
    //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850033010000c842")); // Sends a 54
    //sendMessage(player,MsgUserReliableOrdered18,QByteArray::fromHex("850032010000c842")); // Sends a 54
    sendSetMaxStatRPC(player, 0, 100);
    sendSetStatRPC(player, 0, 100);
    sendSetMaxStatRPC(player, 1, 100);
    sendSetStatRPC(player, 1, 100);



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

void receiveSync(Player& player, QByteArray data) // Receives the 01 updates from each players
{
    // 5 and 6 are id and id>>8
    player.pony.pos.x = dataToFloat(data.right(data.size()  - 11));
    player.pony.pos.y = dataToFloat(data.right(data.size()  - 15));
    player.pony.pos.z = dataToFloat(data.right(data.size()  - 19));

    // TODO : Get the quaternion data and store it (rot.y)
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
