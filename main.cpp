#include <QtWidgets/QApplication>
#include "widget.h"

int argc=0;

QApplication a(argc,(char**)0);
Widget win;

int main(int argc, char *argv[])
{
    // Windows DLL hell fix
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    a.addLibraryPath("platforms");

    win.show();
    win.startServer();

    return a.exec();
}

/**
  TODO :
  - Code the player Id, each player should have a unique Id, and reuse a disconnected player's Id
  - Understand what the 54 messages are for and code them
  - Implement teleporters(vortex)
  - If you create another character with the same name as an existing one, overwrite the existing one
  - Option for Login/Game server ports
**/
