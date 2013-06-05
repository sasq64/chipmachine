#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include "utils.h"

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <tuple>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;


class TelnetServer { //: public Terminal {
public:

	class Session {
	public:

		typedef std::function<void(std::shared_ptr<Session>)> SessionFunction;

		void write(const std::vector<int8_t> &data, int len = -1);
		void write(const std::string &text);

		template <class... A>
		void write(const std::string &fmt, A... args) {
			std::string s = utils::format(fmt, args...);
			write(s);
		}

		Session(NL::Socket *socket = nullptr);

		NL::Socket *getSocket() { return socket; }
		void handleIndata(std::vector<int8_t> &buffer);

		char getChar();
		bool hasChar();
		std::string getLine();
		void startThread(SessionFunction callback, std::shared_ptr<Session> s);

	private:
		NL::Socket *socket;

		enum State {
			NORMAL,
			CR_READ,
			FOUND_IAC,
			OPTION,
			SUB_OPTION,
			FOUND_IAC_SUB	
		};

		State state;
		int8_t option;
		std::vector<int8_t> optionData;
		std::vector<int8_t> inBuffer;
		std::mutex inMutex;
		std::thread sessionThread;

		void setOption(int opt, int val) {
			LOGD("Set option %d %d", opt, val);
		}


		void handleOptionData() {
			LOGD("optionData %d : %d bytes", option, optionData.size());
			optionData.resize(0);
		}
	};

	enum {
		IAC = -1, //ff
		DONT = -2, //fe
		DO = -3, //fd
		WONT = -4, //fc
		WILL = -5,	 //fb
		SB = -6,
		GA = -7,
		EL = -8,
		EC = -9,
		AYT = -10,
		AO = -11,
		IP = -12,
		BRK = -13,
		DM = -14,
		NOP = -15,
		SE = -16,

		LF = 10,
		CR = 13
	};

	enum {
		TRANSMIT_BINARY = 0,
		ECHO = 1,
		SUPRESS_GO_AHEAD = 3,
		TERMINAL_TYPE = 24,
		WINDOW_SIZE = 31
	};

	typedef std::function<void(Session&, const std::vector<std::string>&)> CMDFunction;
	typedef std::function<void(std::shared_ptr<Session>)> SessionFunction;

	TelnetServer(int port);
	void setPrompt(std::string);
	void addCommand(const std::string &cmd, CMDFunction callback);
	void run();
	void runThread();

	void setConnectCallback(SessionFunction callback) {
		connectCallback = callback;
	}

	std::shared_ptr<Session> getSession(NL::Socket* socket) {

		for(auto s : sessions) {
			if(s->getSocket() == socket)
				return s;
		}
		return std::shared_ptr<Session>(nullptr);
	}

	//Session no_session;


private:


	TelnetInit *init;
	NL::Socket socketServer;
	std::string prompt;

	class OnAccept : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	class OnRead : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};


	class OnDisconnect : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	OnAccept onAccept;
	OnRead onRead;
	OnDisconnect onDisconnect;

	NL::SocketGroup group;

	std::thread mainThread;

	std::unordered_map<std::string, CMDFunction> callBacks;
	SessionFunction connectCallback;

	std::vector<std::shared_ptr<Session>> sessions;
};



#endif // TELNET_SERVER_H
