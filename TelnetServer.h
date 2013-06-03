#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include "utils.h"

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <unordered_map>
#include <algorithm>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;

class TelnetServer {
public:

	class User {
	public:
		void write(const std::vector<int8_t> &data) {
			socket->send(&data[0], data.size());
		}

		void write(const std::string &text) {
			socket->send(text.c_str(), text.length());
		}

		template <class... A>
		void write(const std::string &fmt, A... args) {
			std::string s = utils::format(fmt, args...);
			write(s);
		}

		User(NL::Socket *socket = nullptr) : socket(socket), state(NORMAL) {}

		NL::Socket *getSocket() { return socket; }
		void handleIndata(std::vector<int8_t> &buffer);

		char getChar() {
			if(outBuffer.size() > 0) {
				char c = outBuffer[0];
				outBuffer.erase(outBuffer.begin());
				return c;
			}
			return 0;
		}

		bool hasChar() {
			return outBuffer.size() > 0;
		}

		std::string getLine() {
			auto f = std::find(outBuffer.begin(), outBuffer.end(), LF);
			if(f != outBuffer.end()) {
				std::string line = std::string(outBuffer.begin(), f);
				if(line[line.length()-1] == LF);
					line.resize(line.length()-1);
				outBuffer.erase(outBuffer.begin(), ++f);
				return line;
			}
			return "";
		}

		bool hasLine() {
			return std::find(outBuffer.begin(), outBuffer.end(), LF) != outBuffer.end();
		}

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
		std::vector<int8_t> outBuffer;

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

	typedef std::function<void(User&, const std::vector<std::string>&)> CMDFunction;
	typedef std::function<void(User&)> UserFunction;

	TelnetServer(int port);
	void setPrompt(std::string);
	void addCommand(const std::string &cmd, CMDFunction callback);
	void run();
	void runThread();

	void setConnectCallback(UserFunction callback) {
		connectCallback = callback;
	}

	User &getUser(NL::Socket* socket) {

		for(User &u : users) {
			if(u.getSocket() == socket)
				return u;
		}
		return no_user;
	}

	User no_user;


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
	UserFunction connectCallback;

	std::vector<User> users;

	

};

#endif // TELNET_SERVER_H
