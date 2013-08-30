#include "widget.h"
#include "message.h"

void Widget::checkPingTimeouts()
{
    for (int i=0;i<udpPlayers.size();i++)
    {
        if (!udpPlayers[i].connected || !udpPlayers[i].port)
            continue;
        if (((int)timestampNow()-udpPlayers[i].lastPingTime) > pingTimeout)
        {
            logMessage("UDP : Ping timeout ("+QString().setNum(((int)timestampNow()-udpPlayers[i].lastPingTime))+"s), player "+udpPlayers[i].name);
            udpPlayers[i].connected = false;
            sendMessage(udpPlayers[i], MsgDisconnect, "Ping timeout");
        }
    }
}
