#include "TelnetServer.h"
#include "utils.h"
#include "log.h"

//#include <string.h>
//#include <stdio.h>

using namespace std;
using namespace utils;
using namespace logging;


const unsigned SERVER_PORT = 5000;

void TelnetServer::OnAccept::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	NL::Socket* c = socket->accept();
	group->add(c);
	log("Connection from %s:%d\n", c->hostTo(), c->portTo());
	ts->users.push_back(User(c));
	if(ts->connectCallback)
		ts->connectCallback(ts->users.back());
	ts->users.back().write(ts->prompt);
}

void TelnetServer::OnRead::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

	TelnetServer *ts = static_cast<TelnetServer*>(reference);

	char buffer[256];
	buffer[255] = '\0';
	int len = socket->read(buffer, 255);
	//size_t l = strlen(buffer);
	while((len > 0) && ((buffer[len-1] == '\n') || (buffer[len-1] == '\r')))
		len--;
	buffer[len] = 0;
	log("Text from %s:%d: %s\n", socket->hostTo(), socket->portTo(), buffer);

	StringTokenizer st {buffer, " "};
	User user {socket};
	if(st.noParts() > 0) {
		log("Command: %s\n", st.getString(0));
		auto cmd  = ts->callBacks.find(st.getString(0));
		if(cmd != ts->callBacks.end()) {
			vector<string> x;
			for(int i=0; i<st.noParts(); i++) {
				x.push_back(st.getString(i));
			}
			CMDFunction &func = cmd->second;
			
			func(user, x);
		}
	} else
		user.write("Unknown command\n");

	user.write(ts->prompt);
	
	//for(unsigned i=1; i < (unsigned) group->size(); ++i)
	//	if(group->get(i) != socket)
	//		group->get(i)->send(buffer, msgLen + 1);
}


void TelnetServer::OnDisconnect::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	group->remove(socket);
	log("Connection from %s disconnected\n", socket->hostTo());
	delete socket;
}

class TelnetInit {
public:
	TelnetInit() {
		if(!inited)
			NL::init();
		inited = true;
	}
private:	
	static bool inited;
};

bool TelnetInit::inited = false;

TelnetServer::TelnetServer(int port) : init(new TelnetInit()), socketServer(port), prompt(">> ") {

	delete init;
	init = nullptr;

	group.setCmdOnAccept(&onAccept);
	group.setCmdOnRead(&onRead);
	group.setCmdOnDisconnect(&onDisconnect);

	group.add(&socketServer);
}

void TelnetServer::addCommand(const string &cmd, CMDFunction callback) {
	callBacks[cmd] = callback;
}



void TelnetServer::run() {
	while(true) {
		group.listen(2000, this);
	}
}

void TelnetServer::runThread() {
	mainThread = thread {&TelnetServer::run, this};
}



