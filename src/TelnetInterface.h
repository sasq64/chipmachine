#ifndef TELNET_INTERFACE_H
#define TELNET_INTERFACE_H

#include "SongDb.h"
#include <bbsutils/Console.h>

class PlayerInterface;

class TelnetInterface {
public:
	TelnetInterface(PlayerInterface *pi);
	void launchConsole(bbs::Console &console, SongDatabase &db);
	std::mutex playMutex;
private:
	PlayerInterface *player;
};

#endif // TELNET_INTERFACE_H