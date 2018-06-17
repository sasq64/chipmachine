#include "state_machine.h"

namespace statemachine {

std::shared_ptr<BaseCondition> ALWAYS_TRUE = std::make_shared<TrueCondition>();

Condition if_true(bool const& watch)
{
    return make_condition<EQCondition<bool>>(watch, true);
}

Condition if_false(bool const& watch)
{
    return make_condition<EQCondition<bool>>(watch, false);
}

} // namespace statemachine
