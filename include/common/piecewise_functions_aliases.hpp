#ifndef CHAGR_COMMON_PIECEWIESE_FUNCTIONS_ALIASES_HPP
#define CHAGR_COMMON_PIECEWIESE_FUNCTIONS_ALIASES_HPP

#include "common/hyp_lin_function.hpp"
#include "common/linear_function.hpp"
#include "common/piecewise_function.hpp"
#include "common/stateful_function.hpp"
#include "common/interpolating_function.hpp"

namespace charge::common {
using PiecewieseDecHypOrLinFunction = PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing>;
using PiecewieseDecLinearFunction = PiecewieseFunction<LinearFunction, inf_bound, clamp_bound, monotone_decreasing>;
using PiecewieseIncLinearFunction = PiecewieseFunction<LinearFunction, inf_bound, clamp_bound, monotone_increasing>;
using InterpolatingIncFunction = InterpolatingFunction<inf_bound, clamp_bound, monotone_increasing>;
using StatefulPiecewieseDecLinearFunction = StatefulFunction<PiecewieseDecLinearFunction>;
}

#endif
