#ifndef CHARGE_COMMON_FUNCTION_GRAPH_HPP
#define CHARGE_COMMON_FUNCTION_GRAPH_HPP

#include "common/limited_function.hpp"
#include "common/weighted_graph.hpp"

namespace charge::common
{

template <typename FunctionT>
using FunctionGraph = WeightedGraph<LimitedFunction<FunctionT, inf_bound, clamp_bound>>;

}

#endif
