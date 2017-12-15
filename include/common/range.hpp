#ifndef CHARGE_COMMON_ITER_RANGE_HPP
#define CHARGE_COMMON_ITER_RANGE_HPP

namespace charge::common {
template <typename Iter> struct Range final {
    auto begin() const { return begin_; }
    auto end() const { return end_; }
    Iter begin_;
    Iter end_;
};

template <typename Iter> auto make_range(const Iter begin, const Iter end) {
    return Range<Iter>{begin, end};
}
}

#endif
