#ifndef CHARGE_COMMON_LIMITED_FUNCITONS_HPP
#define CHARGE_COMMON_LIMITED_FUNCITONS_HPP

#include "common/hyp_lin_function.hpp"
#include "common/hyperbolic_function.hpp"
#include "common/limited_function.hpp"
#include "common/linear_function.hpp"
#include "common/constant_function.hpp"

namespace charge::common {
using LimitedLinearFunction = LimitedFunction<LinearFunction, inf_bound, clamp_bound>;
using LimitedConstantFunction = LimitedFunction<ConstantFunction, inf_bound, clamp_bound>;
using LimitedHyperbolicFunction = LimitedFunction<HyperbolicFunction, inf_bound, clamp_bound>;
using LimitedHypOrLinFunction = LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>;
}

#endif
