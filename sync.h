#ifndef SYNC_H
#define SYNC_H

#define XMIN -2500
#define YMIN -1000
#define ZMIN -2500

#define XMAX 2500
#define YMAX 2500
#define ZMAX 2500

#define ROTMIN -6.283185
#define ROTMAX 6.283185

#define PosRSSize 16
#define RotRSSize 8

#include <QTimer>
#include <QObject>
#include "character.h"

class Sync : public QObject
{
    Q_OBJECT
public:
    explicit Sync(QObject* parent = 0);
    void startSync();
    void stopSync();
    void sendSyncMessage(Player& source, Player& dest);

public slots:
    void doSync();

private:
    QTimer* syncTimer;
};



#endif // SYNC_H
