#include "sync.h"
#include "widget.h"
#include "message.h"

Sync::Sync(QObject *parent) : QObject(parent)
{
    syncTimer = new QTimer(this);
    connect(syncTimer, SIGNAL(timeout()), this, SLOT(doSync()));
}

void Sync::startSync()
{
    syncTimer->start(win.syncInterval);
}

void Sync::stopSync()
{
    syncTimer->stop();
}

void Sync::doSync()
{
    for (int i=0; i<win.scenes.size(); i++)
    {
        if (win.scenes[i].players.size()<2)
            continue;
        for (int j=0; j<win.scenes[i].players.size(); j++)
            for (int k=0; k<win.scenes[i].players.size(); k++)
            {
                if (j==k)
                    continue;
                sendSyncMessage(win.scenes[i].players[j], win.scenes[i].players[k]);
            }
    }
}

void Sync::sendSyncMessage(Player& source, Player& dest)
{
    QByteArray data(2,0);
    data[0] = source.pony.netviewId;
    data[1] = source.pony.netviewId>>8;
    data += floatToData(timestampNow());
    //data += rangedSingleToData(source.pony.pos.x, XMIN, XMAX, PosRSSize);
    //data += rangedSingleToData(source.pony.pos.y, YMIN, YMAX, PosRSSize);
    //data += rangedSingleToData(source.pony.pos.z, ZMIN, ZMAX, PosRSSize);
    data += floatToData(source.pony.pos.x);
    data += floatToData(source.pony.pos.y);
    data += floatToData(source.pony.pos.z);
    //data += rangedSingleToData(source.pony.rot.x, ROTMIN, ROTMAX, RotRSSize);
    data += rangedSingleToData(source.pony.rot.y, ROTMIN, ROTMAX, RotRSSize);
    //data += rangedSingleToData(source.pony.rot.z, ROTMIN, ROTMAX, RotRSSize);
    sendMessage(dest, MsgUserUnreliable, data);

    //win.logMessage("UDP: Syncing "+QString().setNum(source.pony.netviewId)+" to "+QString().setNum(dest.pony.netviewId));
}
