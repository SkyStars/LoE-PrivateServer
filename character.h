#ifndef CHARACTER_H
#define CHARACTER_H

#include <QString>

class UVector
{
public:
    UVector();
    UVector(float ux, float uy, float uz);

public:
    float x;
    float y;
    float z;
};
typedef struct UVector UVector;

class UQuaternion
{
public:
    UQuaternion();
    UQuaternion(float ux, float uy, float uz, float uw);

public:
    float x;
    float y;
    float z;
    float w;
};
typedef struct UQuaternion UQuaternion;

class Character
{
public:
    Character();

public:
    // Infos
    QString name;
    quint16 id;
    quint16 netviewId;

    // Position/level
    UVector pos;
    UQuaternion rot;
    QString sceneName;
};

class Player : public Character
{
public:
    Player();
    static void savePonies(Player& player,QList<QByteArray> ponies);
    static QList<QByteArray> loadPonies(Player& player);
    static bool savePlayers(QList<Player> playersData);
    static QList<Player> loadPlayers();
    static Player& findPlayer(QList<Player>& players, QString uname);
    static Player& findPlayer(QList<Player>& players, QString uIP, quint16 uport);

public slots:
    void reset();

public:
    QString IP;
    quint16 port;
    QString passhash;
    float lastPingTime;
    int lastPingNumber;
    bool connected;
    quint16 udpSequenceNumbers[32];
    QByteArray *receivedDatas;
    bool inGame;
};

#endif // CHARACTER_H
