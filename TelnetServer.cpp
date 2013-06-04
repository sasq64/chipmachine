#include "log.h"
#include "TelnetServer.h"
#include "utils.h"

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
	LOGD("Connection from %s:%d\n", c->hostTo(), c->portTo());
	ts->sessions.push_back(Session(c));
	Session &s = ts->sessions.back();

	s.write(vector<int8_t>({ IAC, WILL, ECHO }));
	s.write(vector<int8_t>({ IAC, WILL, SUPRESS_GO_AHEAD }));

	if(ts->connectCallback)
		ts->connectCallback(s);
	//s.write(ts->prompt);
}

void TelnetServer::OnRead::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

	TelnetServer *ts = static_cast<TelnetServer*>(reference);

	Session &session = ts->getSession(socket);


	vector<int8_t> buffer;
	buffer.resize(256);
	//vector<int8_t> outBuffer;
	//outBuffer.reserve(256);

	int len = socket->read(&buffer[0], 256);
	buffer.resize(len);
	LOGD("Read %d bytes %d,%d\n", len, buffer[0], buffer[1]);
	session.handleIndata(buffer);

	if(session.hasLine()) {
		string line = session.getLine();
		LOGD("Got line: '%s'", line);

		StringTokenizer st {line, " "};
		if(st.noParts() > 0) {
			log("Command: %s\n", st.getString(0));
			auto cmd  = ts->callBacks.find(st.getString(0));
			if(cmd != ts->callBacks.end()) {
				vector<string> x;
				for(int i=0; i<st.noParts(); i++) {
					x.push_back(st.getString(i));
				}
				CMDFunction &func = cmd->second;
				func(session, x);
			}
		} else
			session.write("Unknown command\r\n");
		session.write(ts->prompt);
	}
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



void TelnetServer::Session::handleIndata(vector<int8_t> &buffer) {

	int start = inBuffer.size();

	for(int8_t b : buffer) {

		if(b < 0 || b == 13 || state != NORMAL) {
			switch(state) {
			case NORMAL:
				if(b == IAC)
					state = FOUND_IAC;
				else if(b == CR)
					state = CR_READ;
				break;
			case CR_READ:
				if(b == 0 || b == 10) {
					inBuffer.push_back(CR);
					inBuffer.push_back(LF);
				} else {
					inBuffer.push_back(CR);
					inBuffer.push_back(b);
				}
				state = NORMAL;
				break;
			case FOUND_IAC:
				if(b < 0 && b >= SE) {
					option = b;
					state = OPTION;
					break;
				}
				inBuffer.push_back(IAC);
				inBuffer.push_back(b);
				break;
			case OPTION:
				if(option == SB) {
					setOption(SB, b);
					state = SUB_OPTION;
				} else {
					setOption(option, b);
					state = NORMAL;
				}
				break;
			case SUB_OPTION:
				if(b == IAC) {
					state = FOUND_IAC_SUB;
				} else {
					optionData.push_back(b);
				}
				break;
			case FOUND_IAC_SUB:
				if(b == SE) {
					handleOptionData();
					state = NORMAL;
				} else {
					optionData.push_back(IAC);
					optionData.push_back(b);
					state = SUB_OPTION;
				}
				break;

			}
		} else
			inBuffer.push_back(b);
	}
	// Local echo
	socket->send(&inBuffer[start], inBuffer.size()-start);
}

