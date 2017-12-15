#ifndef CHARGE_COMMON_SAMPLE_FUNCTION_HPP
#define CHARGE_COMMON_SAMPLE_FUNCTION_HPP

#include "common/constants.hpp"
#include "common/limited_function.hpp"
#include "common/piecewise_function.hpp"

namespace charge::common {
namespace detail {
template <typename FnT, typename OutIter>
auto sample_interval(const double min_x, const double max_x, const double min_y, const double max_y,
                     const FnT &fn, const double x_resolution, const double y_resolution,
                     OutIter out) {
    if (max_x - min_x < x_resolution) {
        *out++ = std::tuple<std::int32_t, std::int32_t>{common::to_upper_fixed(min_x),
                                                        common::to_upper_fixed(min_y)};
        return out;
    }

    assert(min_y >= max_y); // monotone decreasing functions only
    if (min_y - max_y < y_resolution) {
        *out++ = std::tuple<std::int32_t, std::int32_t>{common::to_upper_fixed(min_x),
                                                        common::to_upper_fixed(min_y)};
        return out;
    }

    auto mid_x = (min_x + max_x) / 2;
    auto mid_y = fn(mid_x);
    out = sample_interval(min_x, mid_x, min_y, mid_y, fn, x_resolution, y_resolution, out);
    out = sample_interval(mid_x, max_x, mid_y, max_y, fn, x_resolution, y_resolution, out);

    return out;
}

template <typename FnT, typename OutIter>
auto sample_function(const double min_x, const double max_x, const double min_y, const double max_y,
                     const FnT &fn, const double x_resolution, const double y_resolution,
                     OutIter out) {
    out = sample_interval(min_x, max_x, min_y, max_y, fn, x_resolution, y_resolution, out);

    // Include a sample for max_x if the function is not almost constant
    if (max_x - min_x > x_resolution && min_y - max_y > y_resolution) {
        *out++ = std::tuple<std::int32_t, std::int32_t>{common::to_upper_fixed(max_x),
                                                        common::to_upper_fixed(max_y)};
    }

    return out;
}
}

template <typename FnT, typename MinBoundT, typename MaxBoundT, typename OutIter>
auto sample_function(const LimitedFunction<FnT, MinBoundT, MaxBoundT> &fn,
                     const double x_resolution, const double y_resolution, OutIter out) {
    return detail::sample_function(fn.min_x, fn.max_x, fn(fn.min_x, no_bounds_checks{}),
                                   fn(fn.max_x, no_bounds_checks{}), *fn, x_resolution,
                                   y_resolution, out);

}

template <typename FnT, typename MinBoundT, typename MaxBoundT, typename MonoticityT,
          typename OutIter>
auto sample_function(const PiecewieseFunction<FnT, MinBoundT, MaxBoundT, MonoticityT> &fn,
                     const double x_resolution, const double y_resolution, OutIter out) {
    return detail::sample_function(fn.min_x(), fn.max_x(), fn(fn.min_x()), fn(fn.max_x()), fn,
                                   x_resolution, y_resolution, out);
}

template <typename FnT, typename MinBoundT, typename MaxBoundT, typename MonoticityT,
          typename OutIter>
auto sample_function(const double min_x,
                     const PiecewieseFunction<FnT, MinBoundT, MaxBoundT, MonoticityT> &fn,
                     const double x_resolution, const double y_resolution, OutIter out) {
    return detail::sample_function(min_x, fn.max_x(), fn(min_x), fn(fn.max_x()), fn, x_resolution,
                                   y_resolution, out);
}
}

#endif
