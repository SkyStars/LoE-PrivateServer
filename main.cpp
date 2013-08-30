#include <QtWidgets/QApplication>
#include "widget.h"

int argc=0;

QApplication a(argc,(char**)0);
Widget win;

int main(int argc, char *argv[])
{
    win.show();
    win.startServer();

    return a.exec();
}

/**
  TODO :
  - Code the player Id, each player should have a unique Id, and reuse a disconnected player's Id
  - Clean the unneeded chat message stuff
  - Understand what the 54 messages are for and code them
  - Save/Restore player map and position on connect/disconnect and on server stop/atexit
  - A stop servers button, that switch to Start servers if you click it.
  - Implement teleporters
  - Implement correct game start code
  - Implement the player save system, at least saving of the level/position
  - If you create another character with the same name as an existing one, overwrite the existing one
  - Clean the unneeded operations on tcpPlayers and udpPlayers
  - Move the loginMaxConnected/AlreadyConnected checks into the udp part
  - Option for Login/Game server ports
**/
