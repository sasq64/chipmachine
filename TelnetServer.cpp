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
	ts->users.push_back(User(c));

	ts->users.back().write(vector<int8_t>({ IAC, WILL, ECHO }));
	ts->users.back().write(vector<int8_t>({ IAC, WILL, SUPRESS_GO_AHEAD }));

	if(ts->connectCallback)
		ts->connectCallback(ts->users.back());
	ts->users.back().write(ts->prompt);


}

/*
			state.mode = state match {
				case State(SUB_OPTION, IAC) => FOUND_IAC_SUB
				case State(SUB_OPTION, b) => optionData.put(b); SUB_OPTION
				case State(FOUND_IAC_SUB, SE) => handleOptionData(); NORMAL
				case State(FOUND_IAC_SUB, b) => optionData.put(IAC); optionData.put(b); SUB_OPTION
				case State(OPTION, b) if(option == SB) => setOption(SB, b);  SUB_OPTION
				case State(OPTION, b) => setOption(option, b);  NORMAL
				case State(FOUND_IAC, b) if(b < 0 && b >= SE) => option = b; OPTION
				case State(FOUND_IAC, b) => outBuffer.put(IAC); outBuffer.put(b); NORMAL
				case State(NORMAL, IAC) => FOUND_IAC
				case State(CR_READ, 0) => outBuffer.put(LF) ; NORMAL
				case State(CR_READ, 10) => outBuffer.put(LF) ; NORMAL
				case State(CR_READ, b) => outBuffer.put(CR) ; NORMAL
				case State(NORMAL, CR) => CR_READ
				case State(NORMAL, b) => outBuffer.put(b); NORMAL
			}
*/

void TelnetServer::OnRead::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

	TelnetServer *ts = static_cast<TelnetServer*>(reference);

	User &user = ts->getUser(socket);


	vector<int8_t> buffer;
	buffer.resize(256);
	//vector<int8_t> outBuffer;
	//outBuffer.reserve(256);

	int len = socket->read(&buffer[0], 256);
	buffer.resize(len);
	LOGD("Read %d bytes %d,%d\n", len, buffer[0], buffer[1]);
	user.handleIndata(buffer);

	if(user.hasLine()) {
		string line = user.getLine();
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
				func(user, x);
			}
		} else
			user.write("Unknown command\r\n");
		user.write(ts->prompt);
	}



	//size_t l = strlen(buffer);
	//while((len > 0) && ((buffer[len-1] == '\n') || (buffer[len-1] == '\r')))
	//	len--;
	//buffer[len] = 0;
	/*
	const char *bufferText = static_cast<char*>(&buffer[0]);
	log("Text from %s:%d: %s\n", socket->hostTo(), socket->portTo(), bufferText);

	StringTokenizer st {bufferText, " "};
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
	*/
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



void TelnetServer::User::handleIndata(vector<int8_t> &buffer) {

	int start = outBuffer.size();

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
					outBuffer.push_back(CR);
					outBuffer.push_back(LF);
				} else {
					outBuffer.push_back(CR);
					outBuffer.push_back(b);
				}
				state = NORMAL;
				break;
			case FOUND_IAC:
				if(b < 0 && b >= SE) {
					option = b;
					state = OPTION;
					break;
				}
				outBuffer.push_back(IAC);
				outBuffer.push_back(b);
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
			outBuffer.push_back(b);
	}
	socket->send(&outBuffer[start], outBuffer.size()-start);
}

