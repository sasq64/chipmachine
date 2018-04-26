#include "state_machine.h"

using namespace std;

namespace statemachine {

shared_ptr<BaseCondition> ALWAYS_TRUE = make_shared<TrueCondition>();

Condition if_true(const bool &watch) {
	LOGD("if_true %p", (void*)&watch);
	return make_condition<EQCondition<bool>>(watch, true);
}

Condition if_false(const bool &watch) {
	return make_condition<EQCondition<bool>>(watch, false);
}

} // statemachine
