#ifndef CHARGE_TEST_HELPER_VECTOR_COMPARATOR_HPP
#define CHARGE_TEST_HELPER_VECTOR_COMPARATOR_HPP

#include "common/coordinate.hpp"
#include "common/irange.hpp"

#include <catch.hpp>

#include <string>
#include <vector>

namespace detail {

template <typename T>
class ApproxVector {
   public:
    using value_t = std::vector<T>;

    ApproxVector(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const;

    std::string toString() const {
        return std::string("ApproxVector( ") + Catch::toString(value) +
               std::string(" )");
    }

    double epsilon(double eps_) { eps = eps_; }

    double eps = 0.00001;
    const value_t &value;
};

template <typename T>
bool ApproxVector<T>::operator==(const value_t &other) const {
    bool same = other.size() == value.size();
    if (!same) return same;
    for (auto idx : charge::common::irange<std::size_t>(0, value.size())) {
        same = same && Approx(value[idx]).epsilon(eps) == other[idx];
        if (!same) return same;
    }
    return same;
}

template <>
bool ApproxVector<charge::common::Coordinate>::operator==(
    const value_t &other) const {
    bool same = other.size() == value.size();
    if (!same) return same;
    for (auto idx : charge::common::irange<std::size_t>(0, value.size())) {
        same = same && Approx(value[idx].lat).epsilon(eps) == other[idx].lat;
        if (!same) return same;
        same = same && Approx(value[idx].lon).epsilon(eps) == other[idx].lon;
        if (!same) return same;
    }
    return same;
}

}  // namespace detail

template <typename T>
auto ApproxVector(const std::vector<T> &value) {
    return detail::ApproxVector<T>{value};
}

namespace Catch {
template <>
inline std::string toString<detail::ApproxVector<double>>(
    const detail::ApproxVector<double> &v) {
    return v.toString();
}
}  // namespace Catch

#endif
