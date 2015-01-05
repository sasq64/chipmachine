#ifndef TELNET_INTERFACE_H
#define TELNET_INTERFACE_H


//#include "MusicDatabase.h"
//#include "MusicPlayerList.h"
//#include "ChipMachine.h"
#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <memory>

namespace chipmachine {

class ChipMachine;

class TelnetInterface {
public:
	TelnetInterface(ChipMachine &cm);
	void start();
	void stop();
private:
	//MusicDatabase& db;
	//MusicPlayerList& player;
	ChipMachine &chipmachine;
	std::unique_ptr<bbs::TelnetServer> telnet;
	std::unique_ptr<bbs::Console> console;
};

} // namespace bbs

#endif // TELNET_INTERFACE_H
