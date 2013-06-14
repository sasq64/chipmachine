#include "log.h"
#include "TelnetServer.h"
#include "utils.h"

#include <algorithm>

using namespace std;
using namespace utils;


void TelnetServer::OnAccept::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	NL::Socket* c = socket->accept();
	group->add(c);
	LOGD("Connection from %s:%d\n", c->hostTo(), c->portTo());

	ts->sessions.push_back(make_shared<Session>(c));

	LOGD("Now %d active sessions", ts->sessions.size());

	shared_ptr<Session> session = ts->sessions.back();
	if(ts->connectCallback) {
		session->startThread(ts->connectCallback);
	}
}

void TelnetServer::OnRead::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	auto &session = ts->getSession(socket);

	int len = socket->read(&ts->buffer[0], 32);
	//ts->buffer.resize(len);
	LOGD("Read %d bytes [%02x]\n", len, ts->buffer);
	session.handleIndata(ts->buffer, len);
}


void TelnetServer::OnDisconnect::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	group->remove(socket);
	LOGD("Connection from %s disconnected\n", socket->hostTo());

	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	auto &session = ts->getSession(socket);
	session.close();
	ts->removeSession(session);
	delete socket;
	session.join();
}

// Dummy class to make sure NL:init() is called before the socketServer is created
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

TelnetServer::TelnetServer(int port) : init(new TelnetInit()), no_session(nullptr), socketServer(port), doQuit(false) {

	delete init;
	init = nullptr;

	buffer.resize(32);

	group.setCmdOnAccept(&onAccept);
	group.setCmdOnRead(&onRead);
	group.setCmdOnDisconnect(&onDisconnect);
	group.add(&socketServer);
}

void TelnetServer::run() {
	while(!doQuit) {
		group.listen(500, this);
		for(auto &session : sessions) {
			if(!session->valid()) {
				LOGD("Removing invalid session");
				removeSession(*session.get());
				break;
			}
		}
	}
}

void TelnetServer::stop() {
	telnetMutex.lock();
	doQuit = true;
	telnetMutex.unlock();
	mainThread.join();
}

void TelnetServer::runThread() {
	mainThread = thread {&TelnetServer::run, this};
}

void TelnetServer::Session::handleIndata(vector<int8_t> &buffer, int len) {

	inMutex.lock();

	int start = inBuffer.size();


	for(int i=0; i<len; i++) {
		int8_t b = buffer[i];

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


void TelnetServer::Session::putChar(int c) {	
	write({(char)c}, 1);
}


int TelnetServer::Session::write(const vector<int8_t> &data, int len) {
	if(len == -1) len = data.size();
	socket->send(&data[0], len);
	return len;
}

void TelnetServer::Session::write(const string &text) {
	socket->send(text.c_str(), text.length());
}

//void TelnetServer::Session::handleIndata(vector<int8_t> &buffer);

int TelnetServer::Session::read(std::vector<int8_t> &data, int len) {
	inMutex.lock();
	int rc = inBuffer.size();
	if(rc > 0) {
		data.insert(data.end(), inBuffer.begin(), inBuffer.end());
		inBuffer.resize(0);
	}
	inMutex.unlock();
	return rc;
}


char TelnetServer::Session::getChar() throw(disconnect_excpetion) {
	chrono::milliseconds ms { 100 };
	while(true) {

		if(disconnected)
			throw disconnect_excpetion{};

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

string TelnetServer::Session::getLine() throw(disconnect_excpetion) {
	chrono::milliseconds ms { 100 };
	while(true) {

		if(disconnected)
			throw disconnect_excpetion{};

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

void TelnetServer::Session::close() {
	//closeMe = true;
	disconnected = true;
	socket = nullptr;
}


void TelnetServer::Session::startThread(Session::Callback callback) {

	write(vector<int8_t>({ IAC, DO, TERMINAL_TYPE }));
	//write(vector<int8_t>({ IAC, WILL, ECHO }));
	//write(vector<int8_t>({ IAC, WILL, SUPRESS_GO_AHEAD }));

	sessionThread = thread(callback, std::ref(*this));
}
