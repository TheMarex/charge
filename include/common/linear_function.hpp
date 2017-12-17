#ifndef CHARGE_LIN_FUNCTION_HPP
#define CHARGE_LIN_FUNCTION_HPP

#include "common/function_traits.hpp"

#include <tuple>
#include <limits>

namespace charge {
namespace common {

struct LinearFunction {
    constexpr LinearFunction(double slope, double offset) noexcept : d(slope), b(0), c(offset) {}

    constexpr LinearFunction(double d, double b, double c) noexcept : d(d), b(b), c(c) {}

    LinearFunction() noexcept = default;
    LinearFunction(const LinearFunction &) noexcept = default;
    LinearFunction(LinearFunction &&) noexcept = default;
    LinearFunction &operator=(const LinearFunction &) noexcept = default;
    LinearFunction &operator=(LinearFunction &&) noexcept = default;

    constexpr inline double operator()(const double x) const noexcept{
        if (d == 0.f)
            return c;
        return d * (x - b) + c;
    }

    // y = d*(x-b) + c
    // (y - c) / d + b = x
    // x = 1/d (y - c) + b
    auto inverse() const {
        if (d == 0.0f)
            return LinearFunction(0, std::numeric_limits<double>::infinity());

        return LinearFunction(1.0f / d, c, b);
    }

    double inverse(const double y) const noexcept{
        if (d == 0.0f)
            return std::numeric_limits<double>::infinity();

        return (1.0f / d) * (y - c) + b;
    }

    double deriv(double) const noexcept {
        return d;
    }

    void shift(const double x) { b += x; }

    bool operator==(const LinearFunction &other) const noexcept {
        return std::tie(d, b, c) == std::tie(other.d, other.b, other.c);
    }
    bool operator<(const LinearFunction &other) const noexcept {
        return std::tie(d, b, c) < std::tie(other.d, other.b, other.c);
    }

    double d;
    double b;
    double c;
};

template <> inline bool is_monotone_decreasing(const LinearFunction &lin) { return lin.d <= 0; }

template <> struct is_monotone<LinearFunction> : std::true_type {};

//static_assert(sizeof(LinearFunction) == 12);
}
}

#endif
