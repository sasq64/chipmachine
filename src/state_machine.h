#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <coreutils/log.h>

#include <memory>
#include <deque>
#include <unordered_map>
#include <vector>

#include <cstring>
#include <stdint.h>

namespace statemachine {

class BaseCondition {
public:
	virtual bool check() const = 0;
};

template <class T> class EQCondition : public BaseCondition {
public:
	EQCondition(const T &watch, T val) : watch(watch), val(val) {}
	bool check() const override { return watch == val; }

private:
	const T &watch;
	T val;
};

class TrueCondition : public BaseCondition {
	bool check() const override { return true; }
};

template <class T> class NEQCondition : public BaseCondition {
public:
	NEQCondition(const T &watch, T val) : watch(watch), val(val) {}
	bool check() const override { return watch != val; }

private:
	const T &watch;
	T val;
};

template <class T> class SharedPtrNotNullCondition : public BaseCondition {
public:
	SharedPtrNotNullCondition(const std::shared_ptr<T> &ptr) : ptr(ptr) {}
	bool check() const override { return ptr != nullptr; }

private:
	const std::shared_ptr<T> &ptr;
};

template <class T> class SharedPtrNullCondition : public BaseCondition {
public:
	SharedPtrNullCondition(const std::shared_ptr<T> &ptr) : ptr(ptr) {}
	bool check() const override { return ptr == nullptr; }

private:
	const std::shared_ptr<T> &ptr;
};

std::shared_ptr<BaseCondition> if_true(const bool &watch);
std::shared_ptr<BaseCondition> if_false(const bool &watch);

template <class T> std::shared_ptr<BaseCondition> if_equals(const T &watch, T val) {
	return std::make_shared<EQCondition<T>>(watch, val);
}

template <class T> std::shared_ptr<BaseCondition> if_not_equals(const T &watch, T val) {
	return std::make_shared<NEQCondition<T>>(watch, val);
}

template <class T> std::shared_ptr<BaseCondition> if_not_null(const std::shared_ptr<T> &ptr) {
	return std::make_shared<SharedPtrNotNullCondition<T>>(ptr);
}

template <class T> std::shared_ptr<BaseCondition> if_null(const std::shared_ptr<T> &ptr) {
	return std::make_shared<SharedPtrNullCondition<T>>(ptr);
}

extern std::shared_ptr<BaseCondition> ALWAYS_TRUE;

struct Action {
	Action(uint32_t id = 0, uint32_t event = 0) : id(id), event(event) {}
	uint32_t id;
	uint32_t event;
};

class StateMachine {
public:
	bool put_event(uint32_t event) {
		bool found = false;
		auto &amap = actionmap[event];
		for(const auto &a : amap.actions) {
			if(a.condition->check()) {
				found = true;
				actions.emplace_back(a.action, event);
				if(a.stop)
					break;
			}
		}
		return found;
	}

	// If event e occurs and current state matches sm, perform action a
	void add(uint32_t event, std::shared_ptr<BaseCondition> c, uint32_t action, bool stop = true) {
		ActionSet &a = actionmap[event];
		a.actions.push_back(Mapping(c, action, stop));
	}

	void add(const char *chars, std::shared_ptr<BaseCondition> c, uint32_t action,
	         bool stop = true) {
		for(unsigned i = 0; i < strlen(chars); i++) {
			auto event = chars[i];
			ActionSet &a = actionmap[event];
			a.actions.push_back(Mapping(c, action, stop));
		}
	}

	// Each event (uint32_t) maps to several mappings.
	// Each mapping is a condition and an action.
	// The first mapping with a true condtition will fire
	void add(std::vector<uint32_t> events, std::shared_ptr<BaseCondition> c, uint32_t action,
	         bool stop = true) {
		for(auto event : events) {
			ActionSet &a = actionmap[event];
			a.actions.push_back(Mapping(c, action, stop));
		}
	}
	void add(uint32_t event, uint32_t action, bool stop = true) {
		add(event, ALWAYS_TRUE, action, stop);
	}

	void add(const char *chars, uint32_t action, bool stop = true) {
		add(chars, ALWAYS_TRUE, action, stop);
	}

	void add(std::vector<uint32_t> events, uint32_t action, bool stop = true) {
		add(events, ALWAYS_TRUE, action, stop);
	}

	// template <class T> void add(std::vector<uint32_t> events, Condition<T> c, uint32_t action) {
	//}

	Action next_action() {
		if(actions.size() > 0) {
			auto a = actions.front();
			actions.pop_front();
			return a;
		}
		return Action();
	}
	
	int actionsLeft() { return actions.size(); }

	struct Mapping {
		Mapping(std::shared_ptr<BaseCondition> condition, uint32_t action, bool stop = true)
		    : condition(condition), action(action), stop(stop) {}
		std::shared_ptr<BaseCondition> condition;
		uint32_t action;
		bool stop = false;
	};

	struct ActionSet {
		std::vector<Mapping> actions;
	};

	std::unordered_map<uint32_t, ActionSet> actionmap;
	std::deque<Action> actions;
};

} // statemachine

#endif // STATE_MACHINE_H
