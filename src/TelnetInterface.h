#ifndef TELNET_INTERFACE_H
#define TELNET_INTERFACE_H

//#include "MusicDatabase.h"
//#include "MusicPlayerList.h"
//#include "ChipMachine.h"

namespace bbs {
class TelnetServer;
class Console;
};

#include <memory>

namespace chipmachine {

class MusicPlayerList;

class TelnetInterface {
public:
	TelnetInterface(MusicPlayerList &cm) : player(cm) {}
	void start();
	void stop();

private:
	void runClient(std::shared_ptr<bbs::Console> console);
	// MusicDatabase& db;
	MusicPlayerList &player;
	// ChipMachine &chipmachine;
	std::shared_ptr<bbs::TelnetServer> telnet;
	std::shared_ptr<bbs::Console> console;
};

} // namespace bbs

#endif // TELNET_INTERFACE_H
