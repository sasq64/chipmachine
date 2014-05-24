#ifndef TELNET_INTERFACE_H
#define TELNET_INTERFACE_H


#include "MusicDatabase.h"
#include "MusicPlayerList.h"

#include <bbsutils/telnetserver.h>
#include <bbsutils/console.h>
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>

#include <memory>

namespace chipmachine {

class TelnetInterface {
public:
	TelnetInterface(MusicDatabase& db, MusicPlayerList& player);
	void start();
private:
	MusicDatabase& db;
	MusicPlayerList& player;
	std::unique_ptr<bbs::TelnetServer> telnet;
};

} // namespace bbs

#endif // TELNET_INTERFACE_H
