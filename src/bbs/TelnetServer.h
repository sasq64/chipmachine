#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <utils/format.h>
#include "Terminal.h"

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <exception>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;


class TelnetServer {
public:

	class disconnect_excpetion : public std::exception {
	public:
		virtual const char *what() const throw() { return "Client disconnected"; }
	};

	class Session : public Terminal {
	public:

		typedef std::function<void(Session&)> Callback;

		Session(NL::Socket *socket) : socket(socket), state(NORMAL), localEcho(true), disconnected(false), winWidth(-1), winHeight(-1), termExplored(false) {}
		Session(const Session &s) = delete;
		Session(const Session &&s) : socket(s.socket), state(NORMAL), localEcho(true), disconnected(false), winWidth(-1), winHeight(-1), termExplored(false) {}

		char getChar() throw(disconnect_excpetion);
		bool hasChar() const;
		std::string getLine() throw(disconnect_excpetion);

		void putChar(int c);

		int write(const std::vector<uint8_t> &data, int len = -1) override;
		void write(const std::string &text);
		template <class... A> void write(const std::string &fmt, A... args) {
			std::string s = utils::format(fmt, args...);
			write(s);
		}

		bool valid() { return socket != nullptr; }

	//private

		int read(std::vector<uint8_t> &data, int len = -1) override;

		NL::Socket *getSocket() const { return socket; }
		void handleIndata(std::vector<uint8_t> &buffer, int len);

		void startThread(Callback callback);

		void close();

		void join() {
			sessionThread.join();
		}

		void echo(bool on) {
			localEcho = on;
		}

		virtual int getWidth() const override;
		virtual int getHeight() const override;
		virtual std::string getTermType() const override;

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
		uint8_t option;
		std::vector<uint8_t> optionData;
		std::vector<uint8_t> inBuffer;
		mutable std::mutex inMutex;
		std::thread sessionThread;
		bool localEcho;
		bool disconnected;
		std::string terminalType;
		int winWidth;
		int winHeight;
		mutable bool termExplored;

		void setOption(int opt, int val);
		void handleOptionData();
	};

	// TELNETSERVER

	enum {
		IAC = 0xff, //ff
		DONT = 0xfe, //fe
		DO = 0xfd, //fd
		WONT = 0xfc, //fc
		WILL = 0xfb,	 //fb
		SB = 0xfa,
		GA = 0xf9,
		EL = 0xf8,
		EC = 0xf7,
		AYT = 0xf6,
		AO = 0xf5,
		IP = 0xf4,
		BRK = 0xf3,
		DM = 0xf2,
		NOP = 0xf1,
		SE = 0xf0,

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

	TelnetServer(int port);
	void run();
	void runThread();
	void stop();

	void setOnConnect(Session::Callback callback);
	Session& getSession(NL::Socket* socket);
	void removeSession(const Session &session);

private:

	std::mutex telnetMutex;

	class OnAccept : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	class OnRead : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};


	class OnDisconnect : public NL::SocketGroupCmd {
		void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) override;
	};

	TelnetInit *init;
	Session no_session;
	NL::Socket socketServer;

	OnAccept onAccept;
	OnRead onRead;
	OnDisconnect onDisconnect;

	NL::SocketGroup group;
	std::thread mainThread;
	Session::Callback connectCallback;
	std::vector<uint8_t> buffer;
	std::vector<std::shared_ptr<Session>> sessions;
	bool doQuit;
};



#endif // TELNET_SERVER_H

