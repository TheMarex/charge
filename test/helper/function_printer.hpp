#ifndef CHARGE_TEST_FUNCTION_HELPER_HPP
#define CHARGE_TEST_FUNCTION_HELPER_HPP

#include "common/hyp_lin_function.hpp"
#include "common/hyperbolic_function.hpp"
#include "common/linear_function.hpp"
#include "common/limited_functions_aliases.hpp"

#include "ev/charging_function.hpp"
#include "ev/limited_tradeoff_function.hpp"

#include <catch.hpp>

namespace Catch {
template <>
inline std::string
toString<charge::common::HyperbolicFunction>(const charge::common::HyperbolicFunction &f) {
    std::stringstream ss;
    ss << std::setprecision(12);
    //ss << "HyperbolicFunction{a = " << f.a << ", b = " << f.b << ", c = " << f.c << "}";
    ss << "HyperbolicFunction{" << f.a << ", " << f.b << ", " << f.c << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::LinearFunction>(const charge::common::LinearFunction &f) {
    std::stringstream ss;
    ss << std::setprecision(12);
    //ss << "LinearFunction{d = " << f.d << ", b = " << f.b << ", c = " << f.c << "}";
    ss << "LinearFunction{" << f.d << ", " << f.b << ", " << f.c << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::HypOrLinFunction>(const charge::common::HypOrLinFunction &f) {
    if (f.is_linear())
        return toString(static_cast<const charge::common::LinearFunction&>(f));
    else
        return toString(static_cast<const charge::common::HyperbolicFunction&>(f));
}

template <>
inline std::string
toString<charge::ev::LimitedTradeoffFunction>(const charge::ev::LimitedTradeoffFunction &f) {
    std::stringstream ss;
    ss << std::setprecision(12);
    //ss << "{min_x = " << f.min_x << ", max_x = " << f.max_x << ", " << toString(f.function) << "}";
    ss << "{" << f.min_x << ", " << f.max_x << ", " << toString(f.function) << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::LimitedLinearFunction>(const charge::common::LimitedLinearFunction &f) {
    std::stringstream ss;
    ss << std::setprecision(12);
    //ss << "{min_x = " << f.min_x << ", max_x = " << f.max_x << ", " << toString(f.function) << "}";
    ss << "{" << f.min_x << ", " << f.max_x << ", " << toString(f.function) << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::PiecewieseFunction<charge::common::LinearFunction,
                                            charge::common::inf_bound, charge::common::clamp_bound,
                                            charge::common::monotone_decreasing>>(
    const charge::common::PiecewieseFunction<charge::common::LinearFunction,
                                             charge::common::inf_bound, charge::common::clamp_bound,
                                             charge::common::monotone_decreasing> &pwf) 
{
    std::stringstream ss;
    ss << std::setprecision(12);
    ss << "{";
    for (const auto& function : pwf.functions)
    {
        ss << toString(function);
    }
    ss << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::PiecewieseFunction<charge::common::LinearFunction,
                                            charge::common::inf_bound, charge::common::clamp_bound,
                                            charge::common::monotone_increasing>>(
    const charge::common::PiecewieseFunction<charge::common::LinearFunction,
                                             charge::common::inf_bound, charge::common::clamp_bound,
                                             charge::common::monotone_increasing> &pwf) 
{
    std::stringstream ss;
    ss << std::setprecision(12);
    ss << "{";
    for (const auto& function : pwf.functions)
    {
        ss << toString(function);
    }
    ss << "}";
    return ss.str();
}

template <>
inline std::string
toString<charge::common::PiecewieseFunction<charge::common::HypOrLinFunction,
                                            charge::common::inf_bound, charge::common::clamp_bound,
                                            charge::common::monotone_decreasing>>(
    const charge::common::PiecewieseFunction<charge::common::HypOrLinFunction,
                                             charge::common::inf_bound, charge::common::clamp_bound,
                                             charge::common::monotone_decreasing> &pwf)
{
    std::stringstream ss;
    ss << std::setprecision(12);
    ss << "{";
    for (const auto& function : pwf.functions)
    {
        ss << toString(function);
    }
    ss << "}";
    return ss.str();
}

template <>
inline std::string toString<charge::ev::ChargingFunction>(const charge::ev::ChargingFunction &cf) {
    std::stringstream ss;
    ss << std::setprecision(12);
    ss << "{" << toString(cf.function) << "}";
    return ss.str();
}
}

#endif
