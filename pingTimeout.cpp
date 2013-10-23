#include "widget.h"
#include "message.h"

void Widget::checkPingTimeouts()
{
    //win.logMessage("CHECKING PING TIMEOUT :");
    for (int i=0;i<udpPlayers.size();i++)
    {
        //if (!udpPlayers[i].connected || !udpPlayers[i].port)
        //    continue;
        int time = (timestampNow()-udpPlayers[i].lastPingTime);
        //win.logMessage(QString().setNum(time)+"s");
        if (time > pingTimeout)
        {
            logMessage("UDP: Ping timeout ("+QString().setNum(((int)timestampNow()-udpPlayers[i].lastPingTime))+"s) for "
                       +QString().setNum(udpPlayers[i].pony.netviewId)+" (player "+udpPlayers[i].name+")");
            udpPlayers[i].connected = false;
            sendMessage(udpPlayers[i], MsgDisconnect, "Ping timeout");
            Player::disconnectPlayerCleanup(udpPlayers[i]);
        }
    }
}
