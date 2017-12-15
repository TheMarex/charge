#ifndef CHARGE_LIMITED_FUNCTION_HPP
#define CHARGE_LIMITED_FUNCTION_HPP

#include "common/function_traits.hpp"
#include "common/hyp_lin_function.hpp"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <iostream>

namespace charge::common {
template <typename FunctionT, typename MinBoundT, typename MaxBoundT> struct LimitedFunction {
    double min_x;
    double max_x;
    FunctionT function;

    template <typename... Args>
    LimitedFunction(double min_x, double max_x, Args... args) noexcept
        : min_x(min_x), max_x(max_x), function(std::forward<Args...>(args...)) {}
    LimitedFunction() noexcept = default;

    const FunctionT &operator*() const noexcept { return function; }
    FunctionT &operator*() noexcept { return function; }
    const FunctionT *operator->() const noexcept { return &function; }
    FunctionT *operator->() noexcept { return &function; }

    template <typename FnT> operator LimitedFunction<FnT, MinBoundT, MaxBoundT>() const {
        return LimitedFunction<FnT, MinBoundT, MaxBoundT>{min_x, max_x, static_cast<FnT>(function)};
    }

    inline double operator()(const double x) const noexcept {
        if (x < min_x) {
            if constexpr(std::is_same_v<MinBoundT, clamp_bound>) { return function(min_x); }
            else if constexpr(std::is_same_v<MinBoundT, inf_bound>) {
                return std::numeric_limits<double>::infinity();
            } else {
                throw std::runtime_error("Invalid bounds type");
            }
        }

        if (x > max_x) {
            if constexpr(std::is_same_v<MaxBoundT, clamp_bound>) { return function(max_x); }
            else if constexpr(std::is_same_v<MaxBoundT, inf_bound>) {
                return std::numeric_limits<double>::infinity();
            } else {
                throw std::runtime_error("Invalid bounds type");
            }
        }

        return function(x);
    }

    inline double operator()(const double x, no_bounds_checks) const noexcept {
        assert(x + 0.01 >= min_x);
        assert(x <= max_x + 0.01);
        return function(x);
    }

    auto inverse() const {
        // clang-format off
        if constexpr(common::is_monotone<FunctionT>::value) {
            auto y_1 = function(min_x);
            auto y_2 = function(max_x);
            auto[min_y, max_y] = y_1 < y_2 ? std::tie(y_1, y_2) : std::tie(y_2, y_1);
            auto inverse_function = function.inverse();
            return LimitedFunction<decltype(inverse_function), MinBoundT, MaxBoundT>(
                min_y, max_y, inverse_function);
        } else {
            throw std::runtime_error("Trying to invert a non-monotone function");
        }
        // clang-format on
    }

    auto inverse(const double y) const noexcept{
        return function.inverse(y);
    }

    auto inverse_deriv(const double alpha) const noexcept{
        if constexpr(std::is_same_v<FunctionT, HypOrLinFunction>)
        {
            if (function.is_linear())
            {
                if (std::fabs(static_cast<const LinearFunction&>(function).d - alpha) < 0.0001)
                    return min_x;
                else
                    return std::numeric_limits<double>::infinity();
            }
            else
            {
                assert(function.is_hyperbolic());

                return static_cast<const HyperbolicFunction&>(function).inverse_deriv(alpha);
            }
        }
        else if constexpr(std::is_same_v<FunctionT, LinearFunction>)
        {
            if (std::fabs(function.d - alpha) < 0.0001)
                return min_x;
        }
        else
        {
            return function.inverse_deriv(alpha);
        }
    }

    void limit_from_y(double min_y, double max_y) {
        if (common::is_monotone_decreasing(function)) {
            assert(min_y <= max_y);
            auto current_max_y = function(min_x);
            auto current_min_y = function(max_x);

            const bool clip_min_x = current_max_y > max_y;
            const bool clip_max_x = current_min_y < min_y;

            if (clip_min_x || clip_max_x) {
                const auto inverse = function.inverse();

                if (clip_min_x) {
                    assert(min_x <= inverse(max_y));
                    min_x = inverse(max_y);
                }

                if (clip_max_x) {
                    assert(max_x >= inverse(min_y));
                    max_x = inverse(min_y);
                }
            }
        } else {
            throw std::runtime_error("Trying to limit a non-monotone decreasing function");
        }
    }

    // shifts a function by x to the right
    void shift(double x)
    {
        min_x += x;
        max_x += x;
        function.shift(x);
    }

    // offsets a function by y to the top
    void offset(double y)
    {
        function.offset(y);
    }

    bool operator<(const LimitedFunction &other) const {
        const auto max_y = function(min_x);
        const auto other_max_y = other.function(other.min_x);
        return std::tie(min_x, max_x) < std::tie(other.min_x, other_max_y);
    }

    bool operator==(const LimitedFunction &other) const {
        return std::tie(min_x, max_x, function) ==
               std::tie(other.min_x, other.max_x, other.function);
    }
};

template <typename FunctionT, typename MinBoundT, typename MaxBoundT>
bool is_monotone_decreasing(const LimitedFunction<FunctionT, MinBoundT, MaxBoundT> &f) {
    return is_monotone_decreasing(f.function);
}

template <typename FunctionT, typename MinBoundT, typename MaxBoundT>
struct is_monotone<LimitedFunction<FunctionT, MinBoundT, MaxBoundT>> : std::true_type {};

}

#endif
