#ifndef CHARGE_HYP_FUNCTION_HPP
#define CHARGE_HYP_FUNCTION_HPP

#include "common/function_traits.hpp"

#include <tuple>
#include <cmath>
#include <cassert>

namespace charge {
namespace common {

struct InverseHyperbolicFunction {
    double operator()(double y) const noexcept {
        if (y <= c) return std::numeric_limits<double>::infinity();

        return b + std::sqrt(a / (y - c));
    }

    bool operator==(const InverseHyperbolicFunction &other) const noexcept {
        return std::tie(a, b, c) == std::tie(other.a, other.b, other.c);
    }

    bool operator<(const InverseHyperbolicFunction &other) const noexcept {
        return std::tie(a, b, c) < std::tie(other.a, other.b, other.c);
    }

    double a;
    double b;
    double c;
};

struct HyperbolicFunction {
    double operator()(double x) const noexcept {
        const auto diff = x - b;
        assert(diff > 0);
        return a / (diff * diff) + c;
    }

    double deriv(double x) const noexcept {
        const auto diff = x - b;
        assert(diff > 0);
        return -2 * a / (diff*diff*diff);
    }

    // return the x > b where f'(x) = deriv
    double inverse_deriv(double deriv) const noexcept {
        return b + std::cbrt(-2*a / deriv);
    }

    bool operator==(const HyperbolicFunction &other) const noexcept {
        return std::tie(a, b, c) == std::tie(other.a, other.b, other.c);
    }

    bool operator<(const HyperbolicFunction &other) const noexcept {
        return std::tie(a, b, c) < std::tie(other.a, other.b, other.c);
    }

    auto inverse() const
    {
        return InverseHyperbolicFunction {a, b, c};
    }

    auto inverse(double y) const noexcept {
        if (y <= c) return std::numeric_limits<double>::infinity();

        return b + std::sqrt(a / (y - c));
    }

    void shift(const double x) {
        b += x;
    }

    double a;
    double b;
    double c;
};

template <> inline bool is_monotone_decreasing(const HyperbolicFunction &hyp) { return hyp.a > 0; }

template <> struct is_monotone<HyperbolicFunction> : std::true_type {};

//static_assert(sizeof(HyperbolicFunction) == 12);
}
}

#endif
