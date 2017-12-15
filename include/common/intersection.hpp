#ifndef CHARGE_COMMON_INTERSECTION_HPP
#define CHARGE_COMMON_INTERSECTION_HPP

#include "common/constant_function.hpp"
#include "common/hyp_lin_function.hpp"
#include "common/irange.hpp"
#include "common/limited_function.hpp"
#include "common/roots.hpp"

#include <algorithm>

namespace charge::common {

template <typename OutIter>
auto intersection(const LinearFunction &lhs, const LinearFunction &rhs, OutIter out) {
    if (rhs.d != lhs.d) {
        auto x = (lhs.c - lhs.d * lhs.b - rhs.c + rhs.d * rhs.b) / (rhs.d - lhs.d);
        *out++ = x;
    }

    return out;
}

// There can only be two intersections with the positive half of the hyperbolic function
template <typename OutIter>
auto intersection(const LinearFunction &lhs, const HyperbolicFunction &rhs, OutIter out) {

    if (lhs.d != 0) {
        auto[z_0, z_1, z_2] =
            unique_real_roots(-lhs.d, rhs.c - lhs.c + lhs.b * lhs.d - lhs.d * rhs.b, 0, rhs.a);
        if (z_0 > 1e-3) {
            auto x_0 = *z_0 + rhs.b;
            *out++ = x_0;
        }
        if (z_1 > 1e-3) {
            auto x_1 = *z_1 + rhs.b;
            *out++ = x_1;
        }
        if (z_2 > 1e-3) {
            auto x_2 = *z_2 + rhs.b;
            *out++ = x_2;
        }
    } else if (std::fabs(lhs.c - rhs.c) > 1e-3) {
        assert(lhs.d == 0);
        auto x = rhs.b + std::sqrt(rhs.a / (lhs.c - rhs.c));
        *out++ = x;
    }

    return out;
}

// Two hyperbolic functions can at most have two unique intersection points,
// if we only look at the positive half of them.
template <typename OutIter>
auto intersection(const HyperbolicFunction &lhs, const HyperbolicFunction &rhs, OutIter out) {

    const double a_1 = lhs.a;
    const double b_1 = lhs.b;
    const double c_1 = lhs.c;

    const double a_2 = rhs.a;
    const double b_2 = rhs.b;
    const double c_2 = rhs.c;

    const double dc = c_1 - c_2;
    const double da = a_1 - a_2;
    const double B = b_1 + b_2;
    const double b_1b_1 = b_1 * b_1;
    const double b_2b_2 = b_2 * b_2;
    const double b_1b_2 = b_1 * b_2;

    auto x_min = std::max(b_1, b_2) + 1e-3;

    if (dc != 0) {
        const double a_1dc = a_1 / dc;
        const double a_2dc = a_2 / dc;

        const double a = 1;
        const double b = -2 * B;
        const double c = a_1dc - a_2dc + b_1b_1 + 4 * b_1b_2 + b_2b_2;
        const double d = 2 * a_2dc * b_1 - 2 * a_1dc * b_2 - 2 * b_1 * b_2b_2 - 2 * b_2 * b_1b_1;
        const double e = -a_2dc * b_1b_1 + a_1dc * b_2b_2 + b_1b_1 * b_2b_2;

        auto[x_0, x_1, x_2, x_3] = unique_real_roots(a, b, c, d, e);

        if (x_0 > x_min) {
            *out++ = *x_0;
        }
        if (x_1 > x_min) {
            *out++ = *x_1;
        }
        if (x_2 > x_min) {
            *out++ = *x_2;
        }
        if (x_3 > x_min) {
            *out++ = *x_3;
        }
    } else {
        const double c = da;
        const double d = 2 * (a_1 * b_2 - a_2 * b_1);
        const double e = a_1 * b_2b_2 - a_2 * b_1b_1;
        auto[x_0, x_1] = unique_real_roots(c, d, e);
        if (x_0 > x_min) {
            *out++ = *x_0;
        }
        if (x_1 > x_min) {
            *out++ = *x_1;
        }
    }

    return out;
}

template <typename OutIter>
auto intersection(const LinearFunction &lhs, const HypOrLinFunction &rhs, OutIter out) {
    if (rhs.is_linear()) {
        return intersection(lhs, static_cast<const LinearFunction &>(rhs), out);
    } else {
        assert(rhs.is_hyperbolic());
        return intersection(lhs, static_cast<const HyperbolicFunction &>(rhs), out);
    }
}

template <typename OutIter>
auto intersection(const HypOrLinFunction &lhs, const HypOrLinFunction &rhs, OutIter out) {
    if (lhs.is_linear()) {
        return intersection(static_cast<const LinearFunction &>(lhs), rhs, out);
    } else if (rhs.is_linear()) {
        return intersection(static_cast<const LinearFunction &>(rhs), lhs, out);
    } else {
        assert(lhs.is_hyperbolic());
        assert(rhs.is_hyperbolic());
        return intersection(static_cast<const HyperbolicFunction &>(lhs),
                            static_cast<const HyperbolicFunction &>(rhs), out);
    }
}

// Intersecting two limited functions can at most have 3 intersection points.
// There are two possible intersections within the limits of each function,
// and one possible intersection with the clamped constant function.
template <typename OutIter>
auto intersection(const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> &lhs,
                  const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> &rhs,
                  OutIter out) {
    // FIXME this can be replaced by a filter iterator
    std::array<double, 4> function_intersections;
    auto function_intersections_end = intersection(*lhs, *rhs, function_intersections.begin());

    std::for_each(function_intersections.begin(), function_intersections_end, [&](const double x) {
        if (lhs.min_x <= x && rhs.min_x <= x && lhs.max_x > x && rhs.max_x > x) {
            *out++ = x;
        }
    });

    function_intersections_end =
        intersection(ConstantFunction{lhs(lhs.max_x)}, *rhs, function_intersections.begin());
    if (function_intersections_end != function_intersections.begin()) {
        auto x = function_intersections.front();
        if (x > lhs.max_x && x >= rhs.min_x && x < rhs.max_x) {
            *out++ = x;
        }
    }

    function_intersections_end =
        intersection(ConstantFunction{rhs(rhs.max_x)}, *lhs, function_intersections.begin());
    if (function_intersections_end != function_intersections.begin()) {
        auto x = function_intersections.front();
        if (x > rhs.max_x && x >= lhs.min_x && x < lhs.max_x) {
            *out++ = x;
        }
    }

    return out;
}

inline auto intersection(const LinearFunction &lhs, const LinearFunction &rhs) {
    std::optional<double> maybe_x;
    intersection(lhs, rhs, &maybe_x);
    return maybe_x;
}

inline auto intersection(const LinearFunction &lhs, const HyperbolicFunction &rhs) {
    std::array<std::optional<double>, 3> maybe_x;
    intersection(lhs, rhs, maybe_x.begin());
    return maybe_x;
}

inline auto intersection(const HyperbolicFunction &lhs, const HyperbolicFunction &rhs) {
    std::array<std::optional<double>, 4> maybe_x;
    intersection(lhs, rhs, maybe_x.begin());
    return maybe_x;
}

inline auto intersection(const HypOrLinFunction &lhs, const HypOrLinFunction &rhs) {
    std::array<std::optional<double>, 4> maybe_x;
    intersection(lhs, rhs, maybe_x.begin());
    return maybe_x;
}
inline auto intersection(const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> &lhs,
                         const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> &rhs) {
    std::array<std::optional<double>, 6> maybe_x;
    intersection(lhs, rhs, maybe_x.begin());
    return maybe_x;
}
}

#endif
