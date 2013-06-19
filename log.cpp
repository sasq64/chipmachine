
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <ctime>

namespace logging {

using namespace std;

LogLevel defaultLevel = DEBUG;
static FILE *logFile = nullptr;

void log(const std::string &text) {
	log(defaultLevel, text);
}

void log(LogLevel level, const std::string &text) {
	if(level >= defaultLevel) {
		fwrite(text.c_str(), 1, text.length(), stdout);
		char c = text[text.length()-1];
		if(c != '\n' && c != '\r')
			putc('\n', stdout);			
	}
	if(logFile) {
		
		//std::time_t t = chrono::system_clock::to_time_t(system_clock::now());
		//const std::string s = put_time(t, "%H:%M:%S - ");
		time_t now  = time(nullptr);
		struct tm *t = localtime(&now);
		string ts = utils::format("%02d:%02d.%02d - ", t->tm_hour, t->tm_min, t->tm_sec);

		fwrite(ts.c_str(), 1, ts.length(), logFile);
		fwrite(text.c_str(), 1, text.length(), logFile);
		char c = text[text.length()-1];
		if(c != '\n' && c != '\r')
			putc('\n', logFile);
		fflush(logFile);
	}
}

void log2(const char *fn, int line, LogLevel level, const std::string &text) {
	char temp[2048];
	sprintf(temp, "[%s:%d] ", fn, line);
	log(level, std::string(temp).append(text));
}

void setLevel(LogLevel level) {
	defaultLevel = level;
}
void setOutputFile(const std::string &fileName) {
	if(logFile)
		fclose(logFile);
	logFile = fopen(fileName.c_str(), "wb");
}


}