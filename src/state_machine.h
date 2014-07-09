#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

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
	const T& watch;
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
	const T& watch;
	T val;
};

std::shared_ptr<BaseCondition> if_true(const bool &watch);
std::shared_ptr<BaseCondition> if_false(const bool &watch);

template <class T> std::shared_ptr<BaseCondition> if_equals(const T &watch, T val) {
	return std::make_shared<EQCondition<T>>(watch, val);
}

template <class T> std::shared_ptr<BaseCondition> if_not_equals(const T &watch, T val) {
	return std::make_shared<NEQCondition<T>>(watch, val);
}

extern std::shared_ptr<BaseCondition> ALWAYS_TRUE;

struct Action {
	Action(uint32_t id = 0, uint32_t event = 0) : id(id), event(event) {}
	//operator uint32_t() { return id; }
	uint32_t id;
	uint32_t event;
};

class StateMachine {
public:

	void put_event(uint32_t event) {
		auto &amap = actionmap[event];
		for(const auto &a : amap.actions) {
			if(a.first->check())
				actions.emplace_back(a.second, event);
		}
	}

	// If event e occurs and current state matches sm, perform action a

	void add(uint32_t event, std::shared_ptr<BaseCondition> c, uint32_t action) {
		ActionSet &a = actionmap[event];
		a.actions.push_back(std::make_pair(c, action));
	}

	void add(const char *chars, std::shared_ptr<BaseCondition> c, uint32_t action) {
		for(int i=0; i<strlen(chars); i++) {
			auto event = chars[i];
			ActionSet &a = actionmap[event];
			a.actions.push_back(std::make_pair(c, action));
		}
	}

	void add(std::vector<uint32_t> events, std::shared_ptr<BaseCondition> c, uint32_t action) {
		for(auto event : events) {
			ActionSet &a = actionmap[event];
			a.actions.push_back(make_pair(c, action));
		}
	}
	void add(uint32_t event, uint32_t action) {
		add(event, ALWAYS_TRUE, action);
	}

	void add(const char *chars, uint32_t action) {
		add(chars, ALWAYS_TRUE, action);
	}

	void add(std::vector<uint32_t> events, uint32_t action) {
		add(events, ALWAYS_TRUE, action);
	}

	//template <class T> void add(std::vector<uint32_t> events, Condition<T> c, uint32_t action) {
	//}

	Action next_action() {
		if(actions.size() > 0) {
			auto a = actions.front();
			actions.pop_front();
			return a;
		}
		return Action();
	}

	struct ActionSet {
		std::vector<std::pair<std::shared_ptr<BaseCondition>, uint32_t>> actions;
	};

	std::unordered_map<uint32_t, ActionSet> actionmap;
	std::deque<Action> actions;
};

} // statemachine

#endif // STATE_MACHINE_H
