#ifndef CHARGE_COMMON_FUNCTION_TRAITS_HPP
#define CHARGE_COMMON_FUNCTION_TRAITS_HPP

#include <type_traits>

namespace charge::common {
template <typename FunctionT> bool is_monotone_decreasing(const FunctionT &) {
    return false;
}

template <typename FunctionT> struct is_monotone : std::false_type {};

struct monotone_increasing final {};
struct monotone_decreasing final {};
struct not_monotone final {};

struct inf_bound final {};
struct clamp_bound final {};
struct no_bounds_checks final {};

}

#endif
