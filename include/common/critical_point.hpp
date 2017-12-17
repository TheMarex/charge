#ifndef CHARGE_COMMON_CRITICAL_POINT_HPP
#define CHARGE_COMMON_CRITICAL_POINT_HPP

#include "common/hyp_lin_function.hpp"
#include "common/hyperbolic_function.hpp"
#include "common/linear_function.hpp"

#include <limits>

namespace charge::common {

// Compute the inflection point of the difference between lhs and rhs
// We can use this point to check if these two functions intersect on a given interval
inline double critical_point(const HyperbolicFunction &lhs, const HyperbolicFunction &rhs,
                             const double x_shift = 0.0) {
    const double root = std::cbrt(rhs.a / lhs.a);
    const double x = ((rhs.b + x_shift) - root * lhs.b) / (1.0 - root);
    return x;
}

// For a linear function and a hyperbolic function the inflection point is the point on the
// hyperbolic function that has the same slope as the linear function
inline double critical_point(const HyperbolicFunction &lhs, const LinearFunction &rhs,
                             const double = 0.0) {
    return lhs.inverse_deriv(rhs.d);
}

// symmetric
inline double critical_point(const LinearFunction &lhs, const HyperbolicFunction &rhs,
                             const double x_shift = 0.0) {
    return critical_point(rhs, lhs) + x_shift;
}

// no critical point, both endpoints suffice
inline double critical_point(const LinearFunction &, const LinearFunction &, const double = 0.0) {
    return std::numeric_limits<double>::infinity();
}

inline double critical_point(const HypOrLinFunction &lhs, const HypOrLinFunction &rhs,
                             const double x_shift = 0.0) {
    if (lhs.is_linear()) {
        if (rhs.is_linear()) {
            return critical_point(static_cast<const LinearFunction &>(lhs),
                                  static_cast<const LinearFunction &>(rhs), x_shift);
        } else {
            assert(rhs.is_hyperbolic());
            return critical_point(static_cast<const LinearFunction &>(lhs),
                                  static_cast<const HyperbolicFunction &>(rhs), x_shift);
        }
    } else {
        assert(lhs.is_hyperbolic());

        if (rhs.is_linear()) {
            return critical_point(static_cast<const HyperbolicFunction &>(lhs),
                                  static_cast<const LinearFunction &>(rhs), x_shift);
        } else {
            assert(rhs.is_hyperbolic());
            return critical_point(static_cast<const HyperbolicFunction &>(lhs),
                                  static_cast<const HyperbolicFunction &>(rhs), x_shift);
        }
    }
}

} // namespace charge::common

#endif
