#include "log.h"
#include "TelnetServer.h"
#include "utils.h"

#include <algorithm>

using namespace std;
using namespace utils;

// TELNETSERVER

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

	int len = socket->read(&ts->buffer[0], 128);
	//ts->buffer.resize(len);
	//LOGD("Read %d bytes [%02x]\n", len, make_slice(ts->buffer, 0, len));
	session.handleIndata(ts->buffer, len);
}


void TelnetServer::OnDisconnect::exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
	group->remove(socket);
	LOGD("Connection from %s disconnected\n", socket->hostTo());

	TelnetServer *ts = static_cast<TelnetServer*>(reference);
	auto &session = ts->getSession(socket);
	session.close();

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

	buffer.resize(128);

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

void TelnetServer::setOnConnect(Session::Callback callback) {
	connectCallback = callback;
}

TelnetServer::Session& TelnetServer::getSession(NL::Socket* socket) {

	for(auto &s : sessions) {
		if(s->getSocket() == socket)
			return *s;
	}
	return no_session;
}

void TelnetServer::removeSession(const Session &session) {
	for(auto it = sessions.begin(); it != sessions.end(); ++it) {
		if(it->get()->getSocket() == session.getSocket()) {
			sessions.erase(it);
			return;
		}
	}
}


// TELNETSESSION

void TelnetServer::Session::handleIndata(vector<uint8_t> &buffer, int len) {

	inMutex.lock();

	int start = inBuffer.size();


	for(int i=0; i<len; i++) {
		uint8_t b = buffer[i];

		if(b >= SE || b == 13 || state != NORMAL) {
			switch(state) {
			case NORMAL:
				if(b == IAC)
					state = FOUND_IAC;
				else if(b == CR) {
					inBuffer.push_back(CR);
					state = CR_READ;
				}
				break;
			case CR_READ:
				if(b == 0) {
					inBuffer.push_back(LF);
				} else {
					inBuffer.push_back(b);
				}
				state = NORMAL;
				break;
			case FOUND_IAC:
				if(b >= SE) {
					option = b;
					state = OPTION;
					break;
				}
				inBuffer.push_back(IAC);
				inBuffer.push_back(b);
				break;
			case OPTION:
				if(option == SB) {
					option = b;
					setOption(option, b);
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
	if(disconnected)
		throw disconnect_excpetion{};
	write({(uint8_t)c}, 1);
}


int TelnetServer::Session::write(const vector<uint8_t> &data, int len) {
	if(disconnected)
		throw disconnect_excpetion{};
	if(len == -1) len = data.size();
	socket->send(&data[0], len);
	return len;
}

void TelnetServer::Session::write(const string &text) {
	if(disconnected)
		throw disconnect_excpetion{};
	socket->send(text.c_str(), text.length());
}

//void TelnetServer::Session::handleIndata(vector<uint8_t> &buffer);

int TelnetServer::Session::read(std::vector<uint8_t> &data, int len) {
	if(disconnected)
		throw disconnect_excpetion{};
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
		auto f = find(inBuffer.begin(), inBuffer.end(), CR);
		if(f != inBuffer.end()) {
			string line = string(inBuffer.begin(), f);
			inBuffer.erase(inBuffer.begin(), ++f);
			if(inBuffer.size() > 0 && inBuffer[0] == LF)
				inBuffer.erase(inBuffer.begin());
			inMutex.unlock();

			//if(line[line.length()-1] == CR)
			//	line.pop_back();
				//line.resize(line.length()-1);

			return line;
		}
		inMutex.unlock();

		this_thread::sleep_for(ms);
	}

}

int TelnetServer::Session::getWidth() const  { 
	return winWidth;
}
int TelnetServer::Session::getHeight() const  {
	return winHeight;
}
std::string TelnetServer::Session::getTermType() const  { 
	chrono::milliseconds ms { 100 };
	int delay = 8;
	while(true) {
		if(disconnected)
			throw disconnect_excpetion{};
		if(termExplored)
			return terminalType;
		inMutex.lock();
		inMutex.unlock();
		this_thread::sleep_for(ms);
		if(delay-- == 0) {
			termExplored = true;
		}
	}
}


void TelnetServer::Session::close() {
	//closeMe = true;
	disconnected = true;
	delete socket;
	socket = nullptr;
}


void TelnetServer::Session::startThread(Session::Callback callback) {

	write(vector<uint8_t>({ IAC, DO, TERMINAL_TYPE }));
	sessionThread = thread(callback, std::ref(*this));
}

void TelnetServer::Session::setOption(int opt, int val) {
	LOGD("Set option %d %d", opt, val);
	if(opt == WILL) {
		if(val == TERMINAL_TYPE) {
			write(std::vector<uint8_t>({ IAC, SB, TERMINAL_TYPE, 1, IAC, SE }));
			if(!termExplored) {
				write(vector<uint8_t>({ IAC, WILL, ECHO }));
				write(vector<uint8_t>({ IAC, WILL, SUPRESS_GO_AHEAD }));
				write(vector<uint8_t>({ IAC, DO, WINDOW_SIZE }));
				termExplored = true;
			}
		}
		else if(val == WINDOW_SIZE) 
			write(std::vector<uint8_t>({ IAC, SB, WINDOW_SIZE, 1, IAC, SE }));
	}
}


void TelnetServer::Session::handleOptionData() {
	LOGD("optionData %d : %d bytes", (int)option, optionData.size());
	if(option == TERMINAL_TYPE) {
		terminalType = string(reinterpret_cast<char*>(&optionData[1]), optionData.size()-1);
		LOGD("Termlinal type is '%s'", terminalType);
	} else if(option == WINDOW_SIZE) {
		winWidth = (optionData[0] << 8) | (optionData[1]&0xff);
		winHeight = (optionData[2] << 8) | (optionData[3]&0xff);
		LOGD("Window size is '%d x %d'", winWidth, winHeight);
	}
	optionData.resize(0);
}
