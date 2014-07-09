#include "state_machine.h"

using namespace std;

namespace statemachine {

shared_ptr<BaseCondition> ALWAYS_TRUE = make_shared<TrueCondition>();

shared_ptr<BaseCondition> if_true(const bool &watch) {
	return make_shared<EQCondition<bool>>(watch, true);
}

shared_ptr<BaseCondition> if_false(const bool &watch) {
	return make_shared<EQCondition<bool>>(watch, false);
}

} // statemachine