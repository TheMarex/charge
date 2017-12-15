#ifndef CHARGE_COMMON_CONSTANT_FUNCTION_HPP
#define CHARGE_COMMON_CONSTANT_FUNCTION_HPP

#include "common/linear_function.hpp"

namespace charge::common {

struct ConstantFunction {
    double operator()(double) const noexcept { return c; };

    operator LinearFunction() const noexcept {
        return LinearFunction(0, c);
    }

    double c;
};
}

#endif
