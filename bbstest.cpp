
#include <coreutils/log.h>
#include <bbsutils/TelnetServer.h>
#include <bbsutils/Console.h>

#include <string>

using namespace std;
using namespace bbs;

#if 0
int main(int argc, char **argv) {

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		session.write("hello world\r\n");
		telnet.quit();
	});
	telnet.run();

	return 0;
}
#endif

int main(int argc, char **argv) {

	setvbuf(stdout, NULL, _IONBF, 0);
	logging::setLevel(logging::DEBUG);

	TelnetServer telnet { 12345 };
	telnet.setOnConnect([&](TelnetServer::Session &session) {
		session.echo(false);
		string termType = session.getTermType();		
		LOGD("New connection, TERMTYPE '%s'", termType);

		unique_ptr<Console> console;
		if(termType.length() > 0) {
			console = unique_ptr<Console>(new AnsiConsole { session });
		} else {
			console = unique_ptr<Console>(new PetsciiConsole { session });
		}
		console->flush();

		// Draw chessboard

		console->setFg(Console::WHITE);
		for(int y=0;y<10; y++)
			for(int x=0;x<30; x++) {
				console->setBg((x+y)%2 ? Console::BLUE : Console::WHITE);
				console->put(x+5,y+5," ");
			}
		console->flush();

		// Worm control

		int px = 2;
		int py = 2;
		int d = 0;

		vector<int> dv { 1,0, 0,1, -1,0, 0,-1 };
		console->setBg(Console::YELLOW);
		while(true) {

			console->put(px, py," ");
			console->flush();

			int c = console->getKey(350);
			switch(c) {
			case Console::KEY_LEFT:
				d--;
				break;
			case Console::KEY_RIGHT:
				d++;
				break;
			case Console::KEY_F1:
				console->refresh();
			}
			if(d>3) d-=4;
			if(d<0) d+=4;

			px += dv[d*2];
			py += dv[d*2+1];
		}

		//telnet.stop();
	});
	telnet.run();

	return 0;
}
