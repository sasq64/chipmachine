
#include <string.h>
#include <stdio.h>

using namespace std;
#include <netlink/socket.h>
#include <netlink/socket_group.h>

const unsigned SERVER_PORT = 5000;

class OnAccept: public NL::SocketGroupCmd {

	void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {

		NL::Socket* c = socket->accept();
		group->add(c);
		printf("Connection from %s:%d\n", c->hostTo().c_str(), c->portTo());
	}
};


class OnRead: public NL::SocketGroupCmd {

	void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
		char buffer[256];
		buffer[255] = '\0';
		int len = socket->read(buffer, 255);
		//size_t l = strlen(buffer);
		buffer[len] = 0;
		printf("Text from %s:%d: %s\n", socket->hostTo().c_str(), socket->portTo(), buffer);
		
		//for(unsigned i=1; i < (unsigned) group->size(); ++i)
		//	if(group->get(i) != socket)
		//		group->get(i)->send(buffer, msgLen + 1);
	}
};


class OnDisconnect: public NL::SocketGroupCmd {
	void exec(NL::Socket* socket, NL::SocketGroup* group, void* reference) {
		group->remove(socket);
		printf("Connection from %s disconnected\n", socket->hostTo().c_str());
		delete socket;
	}
};

int server_main() {

	NL::init();

	NL::Socket socketServer(SERVER_PORT);

	NL::SocketGroup group;

	OnAccept onAccept;
	OnRead onRead;
	OnDisconnect onDisconnect;

	group.setCmdOnAccept(&onAccept);
	group.setCmdOnRead(&onRead);
	group.setCmdOnDisconnect(&onDisconnect);

	group.add(&socketServer);

	while(true) {
		group.listen(2000);
	}
	
}
