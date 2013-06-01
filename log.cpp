
#include "log.h"

#include <stdio.h>

namespace logging {

void log(const std::string &text) {
	fwrite(text.c_str(), 1, text.length(), stdout);
}
void log(LogLevel level, const std::string &text) {
	fwrite(text.c_str(), 1, text.length(), stdout);
}

}