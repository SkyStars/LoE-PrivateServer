#ifndef MESSAGE_H
#define MESSAGE_H

#include "character.h"

enum MessageTypes {
    MsgUnconnected = 0,
    MsgUserUnreliable = 1,
    MsgPing = 0x81,
    MsgPong = 0x82,
    MsgConnect = 0x83,
    MsgConnectResponse = 0x84,
    MsgConnectionEstablished = 0x85,
    MsgAcknowledge = 0x86,
    MsgDisconnect = 0x87,
    MsgUserReliableOrdered1 = 0x43,
    MsgUserReliableOrdered2 = 0x44,
    MsgUserReliableOrdered3 = 0x45,
    MsgUserReliableOrdered4 = 0x46,
    MsgUserReliableOrdered5 = 0x47,
    MsgUserReliableOrdered6 = 0x48,
    MsgUserReliableOrdered18 = 0x54,
    MsgUserReliableOrdered32 = 0x62
};

// Public functions
float timestampNow();
QByteArray doubleToData(double num);
QByteArray floatToData(float num);
QByteArray stringToNetData(QString str);
QString netDataToString(QByteArray data);
QByteArray vectorToData(UVector vec);
QByteArray quaternionToData(UQuaternion quat);
void receiveSync(Player& player);
void receiveMessage(Player& player);
void sendMessage(Player& player, quint8 messageType, QByteArray data=QByteArray());
void sendEntitiesList(Player& player);
void sendEntitiesList2(Player& player);
void sendPonies(Player& player);
void sendNetviewInstantiate(Player& player, QString key, quint16 ViewId, quint16 OwnerId, UVector pos, UQuaternion rot);
void sendNetviewInstantiate(Player& player);
void sendNetviewRPC(Player& player);

#endif // MESSAGE_H
