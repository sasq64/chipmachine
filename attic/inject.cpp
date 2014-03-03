#include "inject.h"

#include <string>
#include <unordered_map>


std::unordered_map<std::string, Injection*> injections;

void uninject(const std::string &name, void *i) {
	if(injections.count(name) == 1) {
		Injection *injection = injections[name];
		injection->removeListener(i);
	}
}

/*
void inject_test() {
	inject<std::string>("test", [](std::string &s) -> bool {
		s.append(".x");
		return true;
	});
}


int main(int argc, char **argv) {

	inject_test();

	std::string name = "data";

	injection_point("test", name);

	printf("%s\n", name.c_str());

	assert(name == "data.x");

	return 0;
} */