#include <QFile>
#include <QDir>
#include "widget.h"
#include "character.h"
#include "message.h"

UVector::UVector()
{
    x=0;
    y=0;
    z=0;
}

UVector::UVector(float ux, float uy, float uz)
{
    x=ux;
    y=uy;
    z=uz;
}

UQuaternion::UQuaternion()
{
    x=0;
    y=0;
    z=0;
    w=0;
}

UQuaternion::UQuaternion(float ux, float uy, float uz, float uw)
{
    x=ux;
    y=uy;
    z=uz;
    w=uw;
}

Character::Character()
{
    name = QString();
    id = 0x0085;
    netviewId = 0x0064;
    pos=UVector(0,0,0);
    rot=UQuaternion(0,0,0,0);
    sceneName = QString();
}

Player::Player() : Character()
{
    connected=false;
    inGame=false;
    lastPingNumber=0;
    lastPingTime=timestampNow();
    port=0;
    IP=QString();
    receivedDatas = new QByteArray;
    for (int i=0;i<32;i++)
        udpSequenceNumbers[i]=0;
}

void Player::reset()
{
    name.clear();
    id = 0;
    netviewId = 0;
    pos=UVector(0,0,0);
    rot=UQuaternion(0,0,0,0);
    sceneName.clear();
    connected=false;
    inGame=false;
    lastPingNumber=0;
    lastPingTime=timestampNow();
    port=0;
    IP.clear();
    receivedDatas->clear();
    for (int i=0;i<32;i++)
        udpSequenceNumbers[i]=0;
}

bool Player::savePlayers(QList<Player> playersData)
{
    QFile playersFile("data/players/players.dat");
    if (!playersFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        win.logStatusMessage("Error saving players database");
        win.stopServer();
        return false;
    }

    for (int i=0;i<playersData.size();i++)
    {
        playersFile.write(playersData[i].name.toLatin1());
        playersFile.write("\31");
        playersFile.write(playersData[i].passhash.toLatin1());
        if (i+1!=playersData.size())
            playersFile.write("\n");
    }
    return true;
}

QList<Player> Player::loadPlayers()
{
    QList<Player> players;
    QFile playersFile("data/players/players.dat");
    if (!playersFile.open(QIODevice::ReadOnly))
    {
        win.logStatusMessage("Error reading players database");
        win.stopServer();
        return players;
    }
    QList<QByteArray> data = playersFile.readAll().split('\n');
    if (data.size()==1 && data[0].isEmpty())
    {
        win.logMessage("Player database is empty. Continuing happily");
        return players;
    }
    for (int i=0;i<data.size();i++)
    {
        QList<QByteArray> line = data[i].split('\31');
        if (line.size()!=2)
        {
            win.logStatusMessage("Error reading players database");
            win.stopServer();
            return players;
        }
        Player newPlayer;
        newPlayer.name = line[0];
        newPlayer.passhash = line[1];
        players << newPlayer;
    }
    win.logMessage(QString("Got ")+QString().setNum(players.size())+" players in database");
    return players;
}

Player& Player::findPlayer(QList<Player>& players, QString uname)
{
    for (int i=0; i<players.size(); i++)
    {
        if (players[i].name == uname)
            return players[i];
    }

    Player* emptyPlayer = new Player();
    return *emptyPlayer;
}

Player& Player::findPlayer(QList<Player>& players, QString uIP, quint16 uport)
{
    for (int i=0; i<players.size(); i++)
    {
        if (players[i].IP == uIP && players[i].port == uport)
            return players[i];
    }

    Player* emptyPlayer = new Player();
    return *emptyPlayer;
}

void Player::savePonies(Player& player, QList<QByteArray> ponies)
{
    win.logMessage(QString("UDP : Saving new character data"));

    QDir playerPath(QDir::currentPath());
    playerPath.cd("data");
    playerPath.cd("players");
    playerPath.mkdir(player.name.toLatin1());


    QFile file(QDir::currentPath()+"/data/players/"+player.name.toLatin1()+"/ponies.dat");
    file.open(QIODevice::ReadWrite | QIODevice::Truncate);
    for (int i=0; i<ponies.size(); i++)
    {
        file.write(ponies[i]);
        file.putChar('\31');
    }
}

QList<QByteArray> Player::loadPonies(Player& player)
{
    QList<QByteArray> ponies;
    QFile file(QDir::currentPath()+"/data/players/"+player.name.toLatin1()+"/ponies.dat");
    if (!file.open(QIODevice::ReadOnly))
        return ponies;

    ponies = file.readAll().split('\31');

    for (int i=0;i<ponies.size();i++)
        if (ponies[i].trimmed().isEmpty())
            ponies.removeAt(i);

    return ponies;
}
