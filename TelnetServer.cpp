#include "log.h"
#include "TelnetServer.h"
#include "utils.h"

using namespace std;
using namespace utils;
using namespace logging;

const unsigned SERVER_PORT = 5000;

void TelnetServer::OnAccept::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	NL::Socket* c = socket->accept();
	group->add(c);
	LOGD("Connection from %s:%d\n", c->hostTo(), c->portTo());
	ts->sessions.emplace_back(c);
	Session &session = ts->sessions.back();
	if(ts->connectCallback) {
		session.startThread(ts->connectCallback);
	}
}

void TelnetServer::OnRead::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

	TelnetServer *ts = static_cast<TelnetServer*>(reference);

	auto &session = ts->getSession(socket);

	int len = socket->read(&ts->buffer[0], 256);
	ts->buffer.resize(len);
	LOGD("Read %d bytes %s\n", len, ts->buffer);
	session.handleIndata(ts->buffer);
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

TelnetServer::TelnetServer(int port) : init(new TelnetInit()), no_session(nullptr), socketServer(port) {

	delete init;
	init = nullptr;

	buffer.resize(256);

	group.setCmdOnAccept(&onAccept);
	group.setCmdOnRead(&onRead);
	group.setCmdOnDisconnect(&onDisconnect);
	group.add(&socketServer);
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

	inMutex.lock();

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
	if(localEcho)
		socket->send(&inBuffer[start], inBuffer.size()-start);

	inMutex.unlock();
}



void TelnetServer::Session::write(const vector<int8_t> &data, int len) {
	socket->send(&data[0], data.size());
}

void TelnetServer::Session::write(const string &text) {
	socket->send(text.c_str(), text.length());
}

void TelnetServer::Session::handleIndata(vector<int8_t> &buffer);

char TelnetServer::Session::getChar() {
	chrono::milliseconds ms { 100 };
	while(true) {
		inMutex.lock();
		if(inBuffer.size() > 0)
			break;
		inMutex.unlock();
		this_thread::sleep_for(ms);
	}

	char c = inBuffer[0];
	inBuffer.erase(inBuffer.begin());
	inMutex.unlock();
	return c;
}

bool TelnetServer::Session::hasChar() const {
	inMutex.lock();
	bool rc = inBuffer.size() > 0;
	inMutex.unlock();
	return rc;
}

string TelnetServer::Session::getLine() {
	chrono::milliseconds ms { 100 };
	while(true) {
		inMutex.lock();
		auto f = find(inBuffer.begin(), inBuffer.end(), LF);
		if(f != inBuffer.end()) {
			string line = string(inBuffer.begin(), f);
			inBuffer.erase(inBuffer.begin(), ++f);
			inMutex.unlock();

			if(line[line.length()-1] == CR)
				line.pop_back();
				//line.resize(line.length()-1);

			return line;
		}
		inMutex.unlock();
		this_thread::sleep_for(ms);
	}

}


void TelnetServer::Session::startThread(Session::Callback callback) {

	write(vector<int8_t>({ IAC, WILL, ECHO }));
	write(vector<int8_t>({ IAC, WILL, SUPRESS_GO_AHEAD }));

	sessionThread = thread(callback, std::ref(*this));
}
