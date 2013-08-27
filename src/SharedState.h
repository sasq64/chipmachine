#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <utils/log.h>

#include <utils.h>
#include <unordered_map>

#include <utils/var.h>

class SharedState {
public:

	typedef std::function<void(const std::string &name)> Callback;

	//utils::var& var(const std::string &name) {
	//	return variables[name].variable;
	//}

	utils::var& operator[](const std::string &name) {
		return variables[name].variable;
	}

	void callOnChange(const std::string &name, Callback callback) {
		variables[name].changeCallbacks.push_back(callback);
		variables[name].variable.setCallback([&]() {
			LOGD("Calling %d callbacks for %s", variables[name].changeCallbacks.size(), name);
			for(auto cb : variables[name].changeCallbacks)
				cb(name);
		});
	}

	void touch(const std::string &name) {
		for(auto cb : variables[name].changeCallbacks)
			cb(name);		
	}

	static SharedState& getGlobal(const std::string &name) {
		return globalState[name];
	};

private:

	static std::unordered_map<std::string, SharedState> globalState;

	struct Data {
		utils::var variable;
		std::vector<Callback> changeCallbacks;
	};

	std::unordered_map<std::string, Data> variables;

};

#endif // SHARED_STATE_H>