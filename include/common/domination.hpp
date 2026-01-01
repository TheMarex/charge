#ifndef CHARGE_COMMON_DOMINATION_HPP
#define CHARGE_COMMON_DOMINATION_HPP

#include "common/constants.hpp"
#include "common/critical_point.hpp"
#include "common/limited_function.hpp"
#include "common/piecewise_function.hpp"

namespace charge::common {

template <typename T1, typename T2>
bool epsilon_dominates_lexicographical(const std::tuple<T1, T2> &lhs, const std::tuple<T1, T2> &rhs,
                                       const T1 x_epsilon, const T2 y_epsilon) {
    if (std::get<0>(lhs) - std::get<0>(rhs) <= x_epsilon) {
        return std::get<1>(lhs) - std::get<1>(rhs) <= y_epsilon;
    } else {
        return false;
    }
}

template <typename T1, typename T2>
bool dominates_lexicographical(const std::tuple<T1, T2> &lhs, const std::tuple<T1, T2> &rhs) {
    return epsilon_dominates_lexicographical(lhs, rhs, 0, 0);
}

template <typename FunctionT>
bool epsilon_dominates_limited(const LimitedFunction<FunctionT, inf_bound, clamp_bound> &lhs,
                               const LimitedFunction<FunctionT, inf_bound, clamp_bound> &rhs,
                               const std::uint32_t x_epsilon, const std::uint32_t y_epsilon) {
    if (lhs.min_x <= rhs.min_x + MIN_X_EPSILON * x_epsilon) {
        auto min_smaller = lhs(rhs.min_x) <= rhs(rhs.min_x) + MIN_Y_EPSILON * y_epsilon;
        auto max_smaller = lhs(lhs.max_x) <= rhs(rhs.max_x) + MIN_Y_EPSILON * y_epsilon;
        return min_smaller && max_smaller;
    } else {
        return false;
    }
}

template <typename FunctionT>
bool dominates_limited(const LimitedFunction<FunctionT, inf_bound, clamp_bound> &lhs,
                       const LimitedFunction<FunctionT, inf_bound, clamp_bound> &rhs) {
    return epsilon_dominates_limited(lhs, rhs, 1, 1);
}

enum DominationState : std::uint8_t { UNCLEAR, DOMINATED, UNDOMINATED };

// Monotone decreasing functions can be represented by a triangle that fully encloses them.
// We can do dominance checks on those to quickly figure out if two convex functions could
// dominante eachother.
//
// FIXME: This is actuallys lower when combined with the piecewise test. Probaly too many
// instructions.
//
//    |
//    |        1
//    |\
//    |` \
//  3 | `  \
//    |  4`  \
//    |_____`_.\______
//    |
//    |    2
//    |
template <typename Iter1, typename Iter2>
DominationState dominates_triangle(const Iter1 lhs_begin, const Iter1 lhs_end,
                                   const Iter2 rhs_begin, Iter2 const rhs_end,
                                   std::size_t x_epsilon = 1, std::size_t y_epsilon = 1) {
    if (lhs_begin == lhs_end)
        return DominationState::UNDOMINATED;

    if (rhs_begin == rhs_end)
        return DominationState::DOMINATED;

    const double x_shift = MIN_X_EPSILON * x_epsilon;

    const auto lhs_min_x = lhs_begin->min_x;
    const auto rhs_max_x = std::prev(rhs_end)->max_x;

    // fully in 3
    if (rhs_max_x + x_shift < lhs_min_x)
        return DominationState::UNDOMINATED;

    const double y_shift = MIN_Y_EPSILON * y_epsilon;

    const auto rhs_min_x = rhs_begin->min_x;
    const auto rhs_max_y = (*rhs_begin)(rhs_min_x, no_bounds_checks{});

    const auto lhs_max_x = std::prev(lhs_end)->max_x;
    const auto lhs_min_y = (*std::prev(lhs_end))(lhs_max_x, no_bounds_checks{});

    // fully in 2
    if (rhs_max_y + y_shift < lhs_min_x)
        return DominationState::UNDOMINATED;

    const auto lhs_max_y = (*lhs_begin)(lhs_min_x, no_bounds_checks{});
    const auto rhs_min_y = (*std::prev(rhs_end))(rhs_max_x, no_bounds_checks{});

    const auto lhs_dx = lhs_max_x - lhs_min_x;
    const auto lhs_dy = lhs_min_y - lhs_max_y;
    const auto dx = rhs_min_x + x_shift - lhs_min_x;
    const auto dy = rhs_min_y + y_shift - lhs_max_y;

    const auto cross = [](auto x_1, auto y_1, auto x_2, auto y_2) { return x_1 * y_2 - x_2 * y_1; };

    auto north_of_line = cross(lhs_dx, lhs_dy, dx, dy) >= 0;

    if (north_of_line && rhs_min_y + y_shift >= lhs_min_y)
        return DominationState::DOMINATED;

    return DominationState::UNCLEAR;
}

// Returns the index of the first sub-function of rhs that is undomminated
template <typename Iter1, typename Iter2>
Iter2 find_first_undominated(const Iter1 lhs_begin, const Iter1 lhs_end, const Iter2 rhs_begin,
                             Iter2 const rhs_end, const std::uint32_t x_epsilon = 1,
                             const std::uint32_t y_epsilon = 1) {
    if (lhs_begin == lhs_end)
        return rhs_begin;

    if (rhs_begin == rhs_end)
        return rhs_end;

    // shift rhs to the right and up before checking dominance
    const double x_shift = MIN_X_EPSILON * x_epsilon;
    const double y_shift = MIN_Y_EPSILON * y_epsilon;

    const auto lhs_min_x = rhs_begin->min_x + x_shift;
    const auto rhs_min_x = rhs_begin->min_x;
    if (lhs_begin->min_x > lhs_min_x) {
        return rhs_begin;
    }

    auto lhs_iter = lhs_begin;
    auto rhs_iter = rhs_begin;

    // to avoid numeric problems with adding and subtracting
    // x_shift we use two different coordinate systems for
    // lhs and rhs where lhs_x = rhs_x + x_shift
    auto lhs_x = lhs_min_x;
    auto rhs_x = rhs_min_x;
    while (lhs_iter != lhs_end && lhs_x >= lhs_iter->max_x)
        lhs_iter++;

    while (lhs_iter != lhs_end && rhs_iter != rhs_end) {
        assert(lhs_iter->min_x <= lhs_x);
        assert(rhs_iter->min_x <= rhs_x);
        assert(lhs_iter->max_x >= lhs_x);
        assert(rhs_iter->max_x >= rhs_x);
        const auto lhs_y = (*lhs_iter)(lhs_x, no_bounds_checks{});
        const auto rhs_y = y_shift + (*rhs_iter)(rhs_x, no_bounds_checks{});

        if (lhs_y > rhs_y) {
            // we already check that the min of the segment was dominated
            if (rhs_x > rhs_iter->min_x)
                return rhs_iter;
            else
                return rhs_iter == rhs_begin ? rhs_begin : std::prev(rhs_iter);
        } else {
            auto lhs_critical_x = critical_point(lhs_iter->function, rhs_iter->function, x_shift);
            auto rhs_critical_x = lhs_critical_x - x_shift;
            if (lhs_critical_x < lhs_iter->max_x && rhs_critical_x < rhs_iter->max_x &&
                lhs_critical_x > lhs_x && rhs_critical_x > rhs_x) {
                lhs_x = lhs_critical_x;
                rhs_x = rhs_critical_x;
                continue;
            }
        }

        if (lhs_iter->max_x < rhs_iter->max_x + x_shift) {
            lhs_x = lhs_iter->max_x;
            rhs_x = lhs_x - x_shift;
            lhs_iter++;
        } else {
            lhs_x = rhs_iter->max_x + x_shift;
            rhs_x = rhs_iter->max_x;
            rhs_iter++;
        }
    }

    assert(rhs_end != rhs_begin);
    const auto lhs_max_x = std::prev(rhs_end)->max_x + x_shift;
    const auto rhs_max_x = std::prev(rhs_end)->max_x;
    assert(lhs_end != lhs_begin);
    const auto lhs_min_y = (*std::prev(lhs_end))(lhs_max_x);
    const auto rhs_min_y = (*std::prev(rhs_end))(rhs_max_x);

    while (rhs_iter != rhs_end) {
        auto rhs_y = y_shift + (*rhs_iter)(rhs_x, no_bounds_checks{});
        if (lhs_min_y > rhs_y) {
            if (rhs_x > rhs_iter->min_x)
                return rhs_iter;
            else
                return rhs_iter == rhs_begin ? rhs_begin : std::prev(rhs_iter);
        }
        rhs_x = rhs_iter->max_x;
        rhs_iter++;
    }
    assert(rhs_x == rhs_max_x);

    if (lhs_min_y > MIN_Y_EPSILON * y_epsilon + rhs_min_y)
        return std::prev(rhs_end);

    return rhs_end;
}

// Returns an iterator to one _after_ the last undominated element
// If the iterator is equal to rhs_begin_ there are no undominated segments.
template <typename Iter1, typename Iter2>
Iter2 find_last_undominated(const Iter1 lhs_begin_, const Iter1 lhs_end_, const Iter2 rhs_begin_,
                            Iter2 const rhs_end_, const std::uint32_t x_epsilon = 1,
                            const std::uint32_t y_epsilon = 1) {
    if (lhs_begin_ == lhs_end_)
        return rhs_begin_;

    if (rhs_begin_ == rhs_end_)
        return rhs_end_;

    const auto lhs_rbegin = std::make_reverse_iterator(lhs_end_);
    const auto rhs_rbegin = std::make_reverse_iterator(rhs_end_);
    const auto lhs_rend = std::make_reverse_iterator(lhs_begin_);
    const auto rhs_rend = std::make_reverse_iterator(rhs_begin_);

    assert(lhs_rend != lhs_rbegin);
    assert(rhs_rend != rhs_rbegin);

    const auto to_forward_rhs_iter = [&](const auto iter) {
        assert(rhs_end_ != rhs_begin_);
        return rhs_end_ - std::distance(rhs_rbegin, iter);
    };

    // shift rhs to the right and up before checking dominance
    const double x_shift = MIN_X_EPSILON * x_epsilon;
    const double y_shift = MIN_Y_EPSILON * y_epsilon;

    const auto rhs_max_x = rhs_rbegin->max_x;
    const auto lhs_max_x = lhs_rbegin->max_x;

    // rhs is to the left of lhs
    if (rhs_max_x + x_shift < lhs_begin_->min_x)
        return to_forward_rhs_iter(rhs_rbegin);

    // the minimum of rhs is below lhs
    // last segment can't be dominated
    const auto lhs_min_y = (*lhs_rbegin)(lhs_max_x);
    const auto rhs_min_y = y_shift + (*rhs_rbegin)(rhs_max_x);
    if (lhs_min_y > rhs_min_y) {
        return to_forward_rhs_iter(rhs_rbegin);
    }

    auto lhs_iter = lhs_rbegin;
    auto rhs_iter = rhs_rbegin;

    // To avoid the numeric problems of adding and subtracting
    // x_shift we use two separate coordinate systems
    auto lhs_x = std::min(lhs_max_x, rhs_max_x + x_shift);
    auto rhs_x = std::min(lhs_max_x - x_shift, rhs_max_x);

    // Advance lhs to where rhs starts
    // lhs_max_x > rhs_max_x
    while (lhs_iter != lhs_rend && lhs_iter->min_x > lhs_x) {
        lhs_iter++;
    }

    // we know rhs_max_x is right of lhs_min_x
    assert(lhs_iter != lhs_rend);

    // clip off the part of rhs that are to the right of lhs_max_x
    // lhs_max_x < rhs_max_x
    while (rhs_iter != rhs_rend && rhs_iter->min_x > rhs_x) {
        rhs_iter++;
    }

    if (rhs_iter == rhs_rend)
        return to_forward_rhs_iter(rhs_iter);

    // now we do a synchronized sweep over lhs and rhs
    // and compare the y values to check for possible
    // intersections
    while (lhs_iter != lhs_rend && rhs_iter != rhs_rend) {
        assert(lhs_iter->min_x <= lhs_x);
        assert(rhs_iter->min_x <= rhs_x);
        assert(lhs_iter->max_x >= lhs_x);
        assert(rhs_iter->max_x >= rhs_x);

        auto lhs_y = (*lhs_iter)(lhs_x, no_bounds_checks{});
        auto rhs_y = y_shift + (*rhs_iter)(rhs_x, no_bounds_checks{});

        if (lhs_y > rhs_y) {
            if (rhs_x < rhs_iter->max_x || rhs_iter == rhs_rbegin) {
                return to_forward_rhs_iter(rhs_iter);
            } else {
                return to_forward_rhs_iter(std::prev(rhs_iter));
            }
        } else {
            auto lhs_critical_x = critical_point(lhs_iter->function, rhs_iter->function, x_shift);
            auto rhs_critical_x = lhs_critical_x - x_shift;
            if (lhs_critical_x > lhs_iter->min_x && rhs_critical_x > rhs_iter->min_x &&
                lhs_critical_x < lhs_x && rhs_critical_x < rhs_x) {
                lhs_x = lhs_critical_x;
                rhs_x = rhs_critical_x;

                auto lhs_y = (*lhs_iter)(lhs_x, no_bounds_checks{});
                auto rhs_y = y_shift + (*rhs_iter)(rhs_x, no_bounds_checks{});
                if (lhs_y > rhs_y) {
                    if (rhs_x < rhs_iter->max_x || rhs_iter == rhs_rbegin) {
                        return to_forward_rhs_iter(rhs_iter);
                    } else {
                        return to_forward_rhs_iter(std::prev(rhs_iter));
                    }
                }
            }
        }

        if (lhs_iter->min_x > rhs_iter->min_x + x_shift) {
            lhs_x = lhs_iter->min_x;
            rhs_x = lhs_x - x_shift;
            lhs_iter++;
        } else {
            rhs_x = rhs_iter->min_x;
            lhs_x = rhs_x + x_shift;
            rhs_iter++;
        }
    }

    // check the last coordinate
    if (rhs_iter == rhs_rend) {
        assert(rhs_x == std::prev(rhs_rend)->min_x);

        auto lhs_y = (*lhs_iter)(lhs_x, no_bounds_checks{});
        auto rhs_y = y_shift + (*std::prev(rhs_iter))(rhs_x, no_bounds_checks{});

        if (lhs_y > rhs_y)
            return to_forward_rhs_iter(std::prev(rhs_iter));
    } else {
        // current segment must be undominated
        assert(rhs_iter->min_x < lhs_begin_->min_x);
    }

    return to_forward_rhs_iter(rhs_iter);
}

template <typename RangeIter, typename Iter2>
auto find_undominated_range(const RangeIter lhs_range_begin, const RangeIter lhs_range_end,
                            const Iter2 rhs_begin, const Iter2 rhs_end,
                            const std::uint32_t x_epsilon = 1, const std::uint32_t y_epsilon = 1) {
    auto begin_iter = rhs_begin;
    auto end_iter = rhs_end;

    // Find maximal prefix, front to back
    (void) std::find_if(lhs_range_begin, lhs_range_end, [&](const auto &lhs) {
        begin_iter = find_first_undominated(lhs.begin(), lhs.end(), begin_iter, end_iter, x_epsilon,
                                            y_epsilon);

        return begin_iter == end_iter;
    });

    if (begin_iter == end_iter)
        return std::make_tuple(begin_iter, end_iter);

    // Find maximal suffix, back to front
    auto lhs_range_rbegin = std::make_reverse_iterator(lhs_range_end);
    auto lhs_range_rend = std::make_reverse_iterator(lhs_range_begin);
    assert(std::distance(lhs_range_rbegin, lhs_range_rend) ==
           std::distance(lhs_range_begin, lhs_range_end));
    (void) std::find_if(lhs_range_rbegin, lhs_range_rend, [&](const auto &lhs) {
        end_iter = find_last_undominated(lhs.begin(), lhs.end(), begin_iter, end_iter, x_epsilon,
                                         y_epsilon);

        return begin_iter == end_iter;
    });

    return std::make_tuple(begin_iter, end_iter);
}

template <typename FunctionT>
bool epsilon_dominates_piecewiese(
    const PiecewieseFunction<FunctionT, inf_bound, clamp_bound, monotone_decreasing> &lhs,
    const PiecewieseFunction<FunctionT, inf_bound, clamp_bound, monotone_decreasing> &rhs,
    const std::uint32_t x_epsilon, const std::uint32_t y_epsilon) {
    auto iter =
        find_first_undominated(lhs.functions.begin(), lhs.functions.end(), rhs.functions.begin(),
                               rhs.functions.end(), x_epsilon, y_epsilon);
    return iter == rhs.functions.end();
}

template <typename FunctionT>
bool dominates_piecewiese(
    const PiecewieseFunction<FunctionT, inf_bound, clamp_bound, monotone_decreasing> &lhs,
    const PiecewieseFunction<FunctionT, inf_bound, clamp_bound, monotone_decreasing> &rhs) {
    return epsilon_dominates_piecewiese(lhs, rhs, 1, 1);
}
} // namespace charge::common

#endif
