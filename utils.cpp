#include "widget.h"
#include "ui_widget.h"

void saveResourceToDataFolder(QString resRelPath)
{
    win.logMessage(QString("Saving ")+resRelPath);
    QString dataPath = QDir::homePath()+"/AppData/LocalLow/LoE/Legends of Equestria/";
    QFile::remove(dataPath+resRelPath);
    QFile::copy(QString(":/gameFiles/")+resRelPath, dataPath+resRelPath);
    SetFileAttributesA(QString(dataPath+resRelPath).toStdString().c_str(),FILE_ATTRIBUTE_NORMAL);
}

QByteArray removeHTTPHeader(QByteArray data,QString header)
{
    int i1 = data.indexOf(header);
    if (i1==-1) return data;
    int i2 = data.indexOf("\n", i1);
    if (i2==-1) return data;
    data.remove(i1, i2-i1+1);
    return data;
}
