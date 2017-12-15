#ifndef CHARGE_EV_LIMITED_TRADEOFF_FUNCTION_HPP
#define CHARGE_EV_LIMITED_TRADEOFF_FUNCTION_HPP

#include "common/hyp_lin_function.hpp"
#include "common/constant_function.hpp"
#include "common/limited_function.hpp"
#include "common/piecewise_function.hpp"

namespace charge::ev {

using LimitedTradeoffFunction =
    common::LimitedFunction<common::HypOrLinFunction, common::inf_bound, common::clamp_bound>;
using PiecewieseTradeoffFunction =
    common::PiecewieseFunction<common::HypOrLinFunction, common::inf_bound, common::clamp_bound, common::monotone_decreasing>;
// assert for no struct padding
//static_assert(sizeof(LimitedTradeoffFunction) == 8 + sizeof(common::HypOrLinFunction));

inline LimitedTradeoffFunction make_constant(double duration, double consumption) {
    return LimitedTradeoffFunction{duration, duration, common::ConstantFunction{consumption}};
}
}

#endif
