#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <unordered_map>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;

class TelnetServer {
public:

	class User {
	public:
		void write(const std::string &text) {
			socket->send(text.c_str(), text.length());
		}
		User(NL::Socket *socket) : socket(socket) {}
	private:
		NL::Socket *socket;

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
