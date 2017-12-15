#ifndef CHARGE_COMMON_TUPLE_HELPER_HPP
#define CHARGE_COMMON_TUPLE_HELPER_HPP

#include <tuple>

namespace charge::common {

// sums all elements in the tuple
template <class... Types> decltype(auto) sum(const std::tuple<Types...> &tuple) {
    auto sum_them = [](auto const &... e) -> decltype(auto) { return (e + ...); };
    return std::apply(sum_them, tuple);
}
}

#endif
