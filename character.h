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

class SceneEntity
{
public:
    SceneEntity();

public:
    // Infos
    QString modelName;
    quint16 id;
    quint16 netviewId;

    // Pos
    QString sceneName;
    UVector pos;
    UQuaternion rot;
};

class Pony : public SceneEntity
{
public:
    Pony();

public:
    QByteArray ponyData;
};

class Player
{
public:
    Player();
    static void savePonies(Player& player,QList<Pony> ponies);
    static QList<Pony> loadPonies(Player& player);
    static bool savePlayers(QList<Player> playersData);
    static QList<Player> loadPlayers();
    static Player& findPlayer(QList<Player>& players, QString uname);
    static Player& findPlayer(QList<Player>& players, QString uIP, quint16 uport);
    static void disconnectPlayerCleanup(Player& player);

public slots:
    void reset();

public:
    QString IP;
    quint16 port;
    QString name;
    QString passhash;
    float lastPingTime;
    int lastPingNumber;
    bool connected;
    quint16 udpSequenceNumbers[32];
    QByteArray *receivedDatas;
    Pony pony;
    bool inGame;
};

class WearableItem
{
public:
    WearableItem();

public:
    quint8 index;
    quint32 id;
};

class InventoryItem : public WearableItem
{
public:
    InventoryItem();

public:
    quint32 amount;
};

#endif // CHARACTER_H
