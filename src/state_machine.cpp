#include "state_machine.h"

using namespace std;

namespace statemachine {

shared_ptr<BaseCondition> ALWAYS_TRUE = make_shared<TrueCondition>();

} // statemachine