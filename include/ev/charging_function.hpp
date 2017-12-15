#ifndef CHARGE_EV_CHAGRING_FUNCTION_HPP
#define CHARGE_EV_CHAGRING_FUNCTION_HPP

#include "common/piecewise_function.hpp"
#include "common/linear_function.hpp"
#include "common/stateful_function.hpp"

namespace charge::ev
{
using ChargingFunction = common::StatefulFunction<common::PiecewieseFunction<common::LinearFunction, common::inf_bound, common::clamp_bound, common::monotone_decreasing>>;
}

#endif
