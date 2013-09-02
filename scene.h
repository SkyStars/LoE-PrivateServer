#ifndef SCENE_H
#define SCENE_H

#include <QString>
#include <QList>
#include "dataType.h"

class Vortex
{
public:
    Vortex();

public:
    quint8 id;
    QString destName;
    UVector destPos;
};

class Scene
{
public:
    Scene(QString sceneName);

public:
    QString name;
    QList<Vortex> vortexes;
};

Vortex findVortex(QString sceneName, quint8 id);

#endif // SCENE_H
