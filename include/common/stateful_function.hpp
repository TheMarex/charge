#ifndef CHARGE_COMMON_STATEFUL_FUNCTION_HPP
#define CHARGE_COMMON_STATEFUL_FUNCTION_HPP

#include <functional>

namespace charge::common
{
    // For an invertible function f this implements:
    // g(x, y) = f(f^-1(y) + x)
    // which is a function for which the following property holds:
    // g(x_1, g(x_2, y)) = g(x_1 + x_2, y)
    template<typename FunctionT>
    struct StatefulFunction
    {
        using InverseFunctionT = decltype(std::declval<FunctionT>().inverse());

        StatefulFunction() noexcept = default;
        StatefulFunction(const StatefulFunction&) noexcept = default;
        StatefulFunction(StatefulFunction&&) noexcept = default;
        StatefulFunction& operator=(const StatefulFunction&) noexcept = default;
        StatefulFunction& operator=(StatefulFunction&&) noexcept = default;

        StatefulFunction(FunctionT function_) noexcept
            : function(std::move(function_)),
            inv_function(function.inverse())
        {
        }

        double operator()(const double x, const double y) const
        {
            return function(x + inv_function(y));
        }

        FunctionT function;
        InverseFunctionT inv_function;
    };
}

#endif
