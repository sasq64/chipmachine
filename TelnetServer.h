#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <string>
#include <vector>
#include <functional>

#include <netlink/socket.h>
#include <netlink/socket_group.h>

class TelnetInit;

class TelnetServer {
public:
	TelnetServer(int port);
	void setPrompt(std::string);
	void addCommandListener(const std::string &cmd, std::function<void(std::vector<std::string>)>);
private:
	TelnetInit *init;
	NL::Socket socketServer;
};

#endif // TELNET_SERVER_H
