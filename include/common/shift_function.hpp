#ifndef CHARGE_COMMON_SHIFT_FUNCTION_HPP
#define CHARGE_COMMON_SHIFT_FUNCTION_HPP

#include "common/linear_function.hpp"

namespace charge::common {

// Substracts b from the input
struct ShiftFunction {
    constexpr inline double operator()(double x) noexcept { return x - b; }

    operator LinearFunction() const noexcept { return LinearFunction(1, -b); }

    double b;
};
}

#endif
