#ifndef CHARGE_COMMON_CONVEX_HPP
#define CHARGE_COMMON_CONVEX_HPP

#include <algorithm>

namespace charge::common {
template <typename Iter> Iter find_last_convex(const Iter begin, const Iter end) {
    auto last_deriv = -std::numeric_limits<double>::infinity();
    auto iter = std::find_if(begin, end, [&last_deriv](const auto &f) {
        const auto deriv = f->deriv(f.min_x + 0.05);
        // add epsilon for compensating against rounding errors
        if (deriv + 1e-2 < last_deriv) {
            return true;
        }
        last_deriv = f->deriv(f.max_x - 0.05);
        return false;
    });

    return iter;
}

template <typename Iter> bool is_convex(const Iter begin, const Iter end) {
    return find_last_convex(begin, end) == end;
}
}

#endif
