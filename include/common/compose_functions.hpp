#ifndef CHARGE_COMMON_COMPOSE_FUNCTION_HPP
#define CHARGE_COMMON_COMPOSE_FUNCTION_HPP

#include "common/piecewise_functions_aliases.hpp"
#include "common/constant_function.hpp"
#include "common/hyp_lin_function.hpp"
#include "common/piecewise_function.hpp"
#include "common/stateful_function.hpp"

namespace charge::common {

// Computes g(x - delta(x), f(delta(x))) for a stateful function g
// The result of this is a piecwiese function.

inline auto compose_function(const ConstantFunction &delta, const HypOrLinFunction &f,
                             const StatefulPiecewieseDecLinearFunction &g) {
    auto y = f(delta.c);
    auto x_offset = g.inv_function(y);

    auto result = g.function.clip<HypOrLinFunction>(x_offset);
    result.shift(delta.c - x_offset);
    return result;
}
}

#endif
