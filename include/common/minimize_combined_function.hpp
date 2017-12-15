#ifndef CHARGE_COMMON_MINIMIZE_COMBINED_FUNCTION_HPP
#define CHARGE_COMMON_MINIMIZE_COMBINED_FUNCTION_HPP

#include "common/adapter_iter.hpp"
#include "common/combine_functions.hpp"
#include "common/constant_function.hpp"
#include "common/convex.hpp"
#include "common/limited_functions_aliases.hpp"
#include "common/lower_envelop.hpp"
#include "common/piecewise_functions_aliases.hpp"
#include "common/shift_function.hpp"
#include "common/sink_iter.hpp"

#include <tuple>

namespace charge::common {

using DeltaAndOptimalFunction = std::tuple<LinearFunction, LimitedHypOrLinFunction>;
using Solution = std::array<std::optional<DeltaAndOptimalFunction>, 3>;
using PiecewieseSolution = std::tuple<InterpolatingIncFunction, PiecewieseDecHypOrLinFunction>;

constexpr bool epsilon_less(const double lhs, const double rhs) {
    constexpr double EPSILON = 0.0001;
    return EPSILON < rhs - lhs;
}

namespace detail {
inline LimitedConstantFunction to_constant(const LimitedHypOrLinFunction &f) {
    return LimitedConstantFunction{f.min_x, f.min_x, ConstantFunction{f(f.min_x)}};
}

// Computes delta suitable if we need to swap f and g
//   x - delta(x)
// = x - d*(x - b) - c
// = x * (1 - d) + d*b - c
inline auto swap_delta(const LinearFunction &delta) {
    LinearFunction swapped_delta;
    swapped_delta.d = 1 - delta.d;
    swapped_delta.c = delta.d * delta.b - delta.c;
    swapped_delta.b = 0;
    return swapped_delta;
}

template <typename Iter> inline void swap_solution(Iter begin, Iter end) {

    std::for_each(begin, end, [](auto &solution) {
        auto & [ delta, h ] = solution;
        delta = detail::swap_delta(delta);
    });
}

template <typename Iter, typename OutIter>
auto shift_function(const Iter begin, const Iter end, const LimitedConstantFunction &offset,
                    OutIter out) {
    ShiftFunction shift_delta{offset.min_x};
    DeltaAndOptimalFunction next_solution{shift_delta, {}};
    std::for_each(begin, end, [&](const auto &sub_f) {
        auto & [ delta, h ] = next_solution;
        assert(offset.min_x == offset.max_x);
        auto x_min = sub_f.min_x + offset.min_x;
        auto x_max = sub_f.max_x + offset.min_x;

        if (sub_f->is_linear()) {
            h = LimitedHypOrLinFunction{
                x_min, x_max,
                combine(static_cast<const LinearFunction &>(*sub_f), *offset, shift_delta)};
        } else {
            assert(sub_f->is_hyperbolic());
            h = LimitedHypOrLinFunction{
                x_min, x_max,
                combine(static_cast<const HyperbolicFunction &>(*sub_f), *offset, shift_delta)};
        }

        assert(h->is_linear() || h.min_x > static_cast<const HyperbolicFunction &>(*h).b);

        *out++ = next_solution;
    });

    return out;
}

template <typename FunctionT, typename CombineFn, typename OutIter>
auto combine_piecewise(const PiecewieseDecHypOrLinFunction &f, FunctionT &g,
                       CombineFn combine_function, OutIter out) {
    std::array<DeltaAndOptimalFunction, 3> solutions;
    auto solutions_end = solutions.begin();
    auto to_insert_end = solutions.begin();

    // Combine all sub functions of f until we manged to find g's final position
    auto sub_f_iter = f.functions.begin();
    const auto sub_f_end = f.functions.end();
    assert(sub_f_iter != sub_f_end);
    while (sub_f_iter != sub_f_end) {
        auto &sub_f = *sub_f_iter;

        std::tie(to_insert_end, solutions_end) = combine_function(sub_f, g, solutions);

        std::copy(solutions.begin(), to_insert_end, out);
        sub_f_iter++;

        // The first time we were able to insert all solutions
        // this degenerates to just appending f shifted
        if (to_insert_end == solutions_end) {
            break;
        }
    }

    // in case we ran out of subfunctions of f
    // before we found the right position for g
    std::copy(to_insert_end, solutions_end, out);
    to_insert_end = solutions_end;

    if (sub_f_iter != sub_f_end) {
        // append remaining sub functions of f, shifted in place.
        assert(solutions_end != solutions.begin());
        const auto & [ delta, last ] = *std::prev(solutions_end);
        LimitedConstantFunction offset{
            g.max_x, g.max_x,
            ConstantFunction{last(last.max_x) - (*sub_f_iter)(sub_f_iter->min_x)}};

        return detail::shift_function(sub_f_iter, sub_f_end, offset, out);
    }

    return out;
}
}

// These functions aim to find a minimize
// h(x, delta) := f(delta) + g(x - delta)
// for every x value and the constraint delta <= x.
// For two limited functions f and g.
// The result will be a function delta(x)
// for which h(x, delta(x)) is minimal.

template <typename OutIter, typename FunctionT>
auto combine_minimal(const LimitedFunction<FunctionT, inf_bound, clamp_bound> &f,
                     const LimitedConstantFunction &g, OutIter out) {
    assert(g.min_x == g.max_x);

    const auto x_max = f.max_x + g.max_x;
    const auto x_min = f.min_x + g.min_x;

    ShiftFunction delta{g.min_x};
    *out++ = {delta, {x_min, x_max, combine(*f, *g, delta)}};
    return out;
}

template <typename OutIter, typename FunctionT>
auto combine_minimal(const LimitedConstantFunction &f,
                     const LimitedFunction<FunctionT, inf_bound, clamp_bound> &g, OutIter out) {
    assert(f.min_x == f.max_x);

    const auto x_max = f.max_x + g.max_x;
    const auto x_min = f.min_x + g.min_x;

    ConstantFunction delta{f.min_x};
    *out++ = {delta, {x_min, x_max, combine(*f, *g, delta)}};
    return out;
}

// we need this overload because otherwise constant + constant would be ambigious
template <typename OutIter>
auto combine_minimal(const LimitedConstantFunction &f, const LimitedConstantFunction &g,
                     OutIter out) {
    assert(f.min_x == f.max_x);
    assert(g.min_x == g.max_x);

    const auto x_min = f.min_x + g.min_x;

    ConstantFunction delta{f.min_x};
    *out++ = {delta, {x_min, x_min, combine(*f, *g, delta)}};
    return out;
}

template <typename OutIter>
auto combine_minimal(const LimitedLinearFunction &f, const LimitedLinearFunction &g, OutIter out) {
    if (f->d == 0) {
        if (g->d == 0) {
            return combine_minimal(detail::to_constant(f), detail::to_constant(g), out);
        } else {
            return combine_minimal(detail::to_constant(f), g, out);
        }
    } else if (g->d == 0) {
        return combine_minimal(f, detail::to_constant(g), out);
    }

    const auto x_max = f.max_x + g.max_x;
    const auto x_min = f.min_x + g.min_x;

    // depending on which has a steper decline we first drive slower on the
    // first segement then on the second segment
    if (f->d == g->d) {
        auto slope = (f.max_x - f.min_x) / (x_max - x_min);
        LinearFunction delta{slope, x_min, f.min_x};
        *out++ = {delta, {x_min, x_max, combine(*f, *g, delta)}};
    } else if (f->d > g->d) {
        auto x_mid = f.min_x + g.max_x;

        if (epsilon_less(x_min, x_mid)) {
            ConstantFunction delta{f.min_x};
            *out++ = {delta, {x_min, x_mid, combine(*f, *g, delta)}};
        } else {
            x_mid = x_min;
        }

        if (epsilon_less(x_mid, x_max)) {
            ShiftFunction delta{g.max_x};
            *out++ = {delta, {x_mid, x_max, combine(*f, *g, delta)}};
        }
    } else {
        assert(f->d < g->d);
        auto x_mid = f.max_x + g.min_x;

        if (epsilon_less(x_min, x_mid)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid, combine(*f, *g, delta)}};
        } else {
            x_mid = x_min;
        }

        if (epsilon_less(x_mid, x_max)) {
            ConstantFunction delta{f.max_x};
            *out++ = {delta, {x_mid, x_max, combine(*f, *g, delta)}};
        }
    }

    return out;
}

template <typename OutIter>
auto combine_minimal(const LimitedHyperbolicFunction &f, const LimitedLinearFunction &g,
                     OutIter out) {

    const auto x_max = f.max_x + g.max_x;
    const auto x_min = f.min_x + g.min_x;

    if (g->d == 0.0) {
        return combine_minimal(f, detail::to_constant(g), out);
    }
    assert(g->d != 0);

    const auto d_star = f->b + std::cbrt(-2 * f->a / g->d);

    // implies f'(x) > g'(x) on [x_min, x_max] so we first drive slower on g then on f
    if (d_star < f.min_x) {
        auto x_mid = f.min_x + g.max_x;

        if (epsilon_less(x_min, x_mid)) {
            ConstantFunction delta{f.min_x};
            *out++ = {delta, {x_min, x_mid, combine(*f, *g, delta)}};
        } else {
            x_mid = x_min;
        }

        if (epsilon_less(x_mid, x_max)) {
            ShiftFunction delta{g.max_x};
            *out++ = {delta, {x_mid, x_max, combine(*f, *g, delta)}};
        }
    }
    // implies f'(x) < g'(x) on [x_min, x_max] so we first drive slower on f then on g
    else if (d_star > f.max_x) {
        auto x_mid = f.max_x + g.min_x;

        if (epsilon_less(x_min, x_mid)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid, combine(*f, *g, delta)}};
        } else {
            x_mid = x_min;
        }

        if (epsilon_less(x_mid, x_max)) {
            ConstantFunction delta{f.max_x};
            *out++ = {delta, {x_mid, x_max, combine(*f, *g, delta)}};
        }
    }
    // implies f'(x) < g'(x) on [x_min, d_star] and f'(x) > g'(x) on [d_star, x_max]
    // so we first drive slower on f then on g and then on f again
    else {
        auto x_mid_1 = d_star + g.min_x;
        auto x_mid_2 = d_star + g.max_x;

        if (epsilon_less(x_min, x_mid_1)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid_1, combine(*f, *g, delta)}};
        } else {
            x_mid_1 = x_min;
        }

        if (epsilon_less(x_mid_1, x_mid_2)) {
            ConstantFunction delta{d_star};
            *out++ = {delta, {x_mid_1, x_mid_2, combine(*f, *g, delta)}};
        } else {
            x_mid_2 = x_mid_1;
        }

        if (epsilon_less(x_mid_2, x_max)) {
            ShiftFunction delta{g.max_x};
            *out++ = {delta, {x_mid_2, x_max, combine(*f, *g, delta)}};
        }
    }

    return out;
}

template <typename OutIter>
auto combine_minimal(const LimitedLinearFunction &f, const LimitedHyperbolicFunction &g,
                     OutIter out) {
    std::array<DeltaAndOptimalFunction, 3> solutions;
    auto end = combine_minimal(g, f, solutions.begin());
    detail::swap_solution(solutions.begin(), end);
    return std::copy(std::make_move_iterator(solutions.begin()), std::make_move_iterator(end), out);
}

template <typename OutIter>
OutIter combine_minimal(const LimitedHyperbolicFunction &f, const LimitedHyperbolicFunction &g,
                        OutIter out) {
    const auto f_min_deriv = -2 * f->a / std::pow(f.min_x - f->b, 3);
    const auto g_min_deriv = -2 * g->a / std::pow(g.min_x - g->b, 3);

    if (f_min_deriv > g_min_deriv) {
        std::array<DeltaAndOptimalFunction, 3> solutions;
        auto end = combine_minimal(g, f, solutions.begin());
        detail::swap_solution(solutions.begin(), end);
        return std::copy(std::make_move_iterator(solutions.begin()), std::make_move_iterator(end),
                         out);
    }

    const auto f_max_deriv = -2 * f->a / std::pow(f.max_x - f->b, 3);
    const auto g_max_deriv = -2 * g->a / std::pow(g.max_x - g->b, 3);
    const auto x_max = f.max_x + g.max_x;
    const auto x_min = f.min_x + g.min_x;

    const auto d_star_d = 1.0f / (1.0f + std::cbrt(g->a / f->a));
    const auto d_star_b = g->b - f->b * std::cbrt(g->a / f->a);
    const auto d_star_c = 0;

    // because we enforce f_min_deriv < g_min_deriv these values are never needed
    // d_start(x) > f.min_x
    // auto f_min_x_star =
    //    f.min_x + g->b + std::cbrt(g->a / f->a) * (f.min_x - f->b);

    // d_start(x) < f.max_x
    const auto f_max_x_star = f.max_x + g->b + std::cbrt(g->a / f->a) * (f.max_x - f->b);

    // d_star(x) > x - g.min_x
    const auto g_min_x_star = g.min_x + f->b + std::cbrt(f->a / g->a) * (g.min_x - g->b);
    // d_star(x) < x - g.max_x
    const auto g_max_x_star = g.max_x + f->b + std::cbrt(f->a / g->a) * (g.max_x - g->b);

    if (g_min_deriv <= f_max_deriv && f_max_deriv < g_max_deriv) {
        auto x_mid_1 = g_min_x_star;
        auto x_mid_2 = f_max_x_star;
        if (epsilon_less(x_min, x_mid_1)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid_1, combine(*f, *g, delta)}};
        } else {
            x_mid_1 = x_min;
        }
        if (epsilon_less(x_mid_1, x_mid_2)) {
            LinearFunction delta{d_star_d, d_star_b, d_star_c};
            *out++ = {delta, {x_mid_1, x_mid_2, combine(*f, *g, delta)}};
        } else {
            x_mid_2 = x_mid_1;
        }
        if (epsilon_less(x_mid_2, x_max)) {
            ConstantFunction delta{f.max_x};
            *out++ = {delta, {x_mid_2, x_max, combine(*f, *g, delta)}};
        }
    } else if (f_max_deriv <= g_min_deriv) {
        auto x_mid = f.max_x + g.min_x;
        if (epsilon_less(x_min, x_mid)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid, combine(*f, *g, delta)}};
        } else {
            x_mid = x_min;
        }
        if (epsilon_less(x_mid, x_max)) {
            ConstantFunction delta{f.max_x};
            *out++ = {delta, {x_mid, x_max, combine(*f, *g, delta)}};
        }
    } else if (g_max_deriv <= f_max_deriv) {
        auto x_mid_1 = g_min_x_star;
        auto x_mid_2 = g_max_x_star;
        if (epsilon_less(x_min, x_mid_1)) {
            ShiftFunction delta{g.min_x};
            *out++ = {delta, {x_min, x_mid_1, combine(*f, *g, delta)}};
        } else {
            x_mid_1 = x_min;
        }
        if (epsilon_less(x_mid_1, x_mid_2)) {
            LinearFunction delta{d_star_d, d_star_b, d_star_c};
            *out++ = {delta, {x_mid_1, x_mid_2, combine(*f, *g, delta)}};
        } else {
            x_mid_2 = x_mid_1;
        }
        if (epsilon_less(x_mid_2, x_max)) {
            ShiftFunction delta{g.max_x};
            *out++ = {delta, {x_mid_2, x_max, combine(*f, *g, delta)}};
        }
    }

    return out;
}

template <typename OutIter>
auto combine_minimal(const LimitedHypOrLinFunction &f, const LimitedHypOrLinFunction &g,
                     OutIter out) {
    // Handle numeric noise as constant values when linking
    if (!epsilon_less(f.min_x, f.max_x)) {
        if (epsilon_less(g.min_x, g.max_x)) {
            return combine_minimal(detail::to_constant(f), g, out);
        } else {
            return combine_minimal(detail::to_constant(f), detail::to_constant(g), out);
        }
    }
    if (!epsilon_less(g.min_x, g.max_x)) {
        assert(epsilon_less(f.min_x, f.max_x));
        return combine_minimal(f, detail::to_constant(g), out);
    }

    assert(epsilon_less(f.min_x, f.max_x));
    assert(epsilon_less(g.min_x, g.max_x));

    if (f->is_linear()) {
        if (g->is_linear()) {
            return combine_minimal(static_cast<LimitedLinearFunction>(f),
                                   static_cast<LimitedLinearFunction>(g), out);
        } else {
            return combine_minimal(static_cast<LimitedLinearFunction>(f),
                                   static_cast<LimitedHyperbolicFunction>(g), out);
        }
    } else {
        if (g->is_linear()) {
            return combine_minimal(static_cast<LimitedHyperbolicFunction>(f),
                                   static_cast<LimitedLinearFunction>(g), out);
        } else {
            return combine_minimal(static_cast<LimitedHyperbolicFunction>(f),
                                   static_cast<LimitedHyperbolicFunction>(g), out);
        }
    }
}

// We need to pass g by value since we need to modify its intervals
template <typename OutIter>
auto combine_minimal(const PiecewieseDecHypOrLinFunction &f, LimitedHyperbolicFunction g,
                     OutIter out) {
    return detail::combine_piecewise(
        f, g,
        [](const auto &sub_f, auto &g, auto &solutions) {
            using iter = typename std::remove_reference<decltype(solutions)>::type::iterator;
            iter solutions_end;
            iter to_insert_end;

            if (sub_f->is_linear()) {
                solutions_end = combine_minimal(static_cast<LimitedLinearFunction>(sub_f), g,
                                                solutions.begin());
                assert(solutions_end != solutions.begin());
                auto & [ delta, last ] = *std::prev(solutions_end);
                if (last->is_linear()) {
                    // we are finished with searching for g, it is already at its optimal position
                    assert(static_cast<const LinearFunction &>(*last).d ==
                           static_cast<const LinearFunction &>(*sub_f).d);
                    g.min_x = g.max_x;
                    to_insert_end = solutions_end;
                } else {
                    assert(last->is_hyperbolic());

                    const auto &last_hyp = static_cast<const HyperbolicFunction &>(*last);
                    assert(last_hyp.a == g->a);
                    // reconstruct the x-value on the unshifted g
                    auto g_min_x_star = (last.min_x - last_hyp.b) + g->b;

                    // we are not finished with g
                    if (epsilon_less(g_min_x_star, g.max_x)) {
                        // assert(!epsilon_less(g_min_x_star, g.min_x));
                        g.min_x = std::max(g.min_x, g_min_x_star);
                        // we placed g at the end, we need to continue
                        to_insert_end = std::prev(solutions_end);
                    } else {
                        g.min_x = g.max_x;
                        to_insert_end = solutions_end;
                    }
                }
            } else {
                assert(sub_f->is_hyperbolic());
                solutions_end = combine_minimal(static_cast<LimitedHyperbolicFunction>(sub_f), g,
                                                solutions.begin());
                assert(solutions_end != solutions.begin());
                auto & [ delta, last ] = *std::prev(solutions_end);
                assert(last->is_hyperbolic());

                auto f_max_deriv = sub_f->deriv(sub_f.max_x);
                auto last_max_deriv = last->deriv(last.max_x);
                // in this case the last segment did not contain
                // parts of f and we can discard it.
                if (last_max_deriv > f_max_deriv + 1e-3) {
                    const auto &last_hyp = static_cast<const HyperbolicFunction &>(*last);
                    assert(last_hyp.a == g->a);
                    // reconstruct the x-value on the unshifted g
                    auto g_min_x_star = (last.min_x - last_hyp.b) + g->b;

                    // we are not finished with g
                    if (epsilon_less(g_min_x_star, g.max_x)) {
                        // assert(!epsilon_less(g_min_x_star, g.min_x));
                        g.min_x = std::max(g.min_x, g_min_x_star);
                        // we placed g at the end, we need to continue
                        to_insert_end = std::prev(solutions_end);
                    } else {
                        g.min_x = g.max_x;
                        to_insert_end = solutions_end;
                    }
                } else {
                    // we found the right place for g, the last
                    // segment already contained f
                    g.min_x = g.max_x;
                    to_insert_end = solutions_end;
                }
            }

            assert(std::all_of(solutions.begin(), to_insert_end, [](const auto &s) {
                auto & [ d, h ] = s;
                return h->is_linear() || h.min_x > static_cast<const HyperbolicFunction &>(*h).b;
            }));

            return std::make_tuple(to_insert_end, solutions_end);
        },
        out);
}

template <typename OutIter>
auto combine_minimal(const PiecewieseDecHypOrLinFunction &f, const LimitedLinearFunction &g,
                     OutIter out) {
    return detail::combine_piecewise(
        f, g,
        [](const auto &sub_f, const auto &g, auto &solutions) {
            using iter = typename std::remove_reference<decltype(solutions)>::type::iterator;
            iter solutions_end;
            iter to_insert_end;

            if (sub_f->is_linear()) {
                solutions_end = combine_minimal(static_cast<LimitedLinearFunction>(sub_f), g,
                                                solutions.begin());
                assert(solutions_end != solutions.begin());
                auto & [ delta, last ] = *std::prev(solutions_end);
                assert(last->is_linear());
                const auto &last_lin = static_cast<const LinearFunction &>(*last);

                if (last_lin.d == g->d) {
                    // last segment is just a shifted g, discard since we might find
                    // a better position for it.
                    to_insert_end = std::prev(solutions_end);
                } else {
                    // we are finished with linking g, it is already at its optimal position
                    assert(last_lin.d == static_cast<LinearFunction>(*sub_f).d);
                    to_insert_end = solutions_end;
                }
            } else {
                assert(sub_f->is_hyperbolic());
                solutions_end = combine_minimal(static_cast<LimitedHyperbolicFunction>(sub_f), g,
                                                solutions.begin());
                assert(solutions_end != solutions.begin());
                auto & [ delta, last ] = *std::prev(solutions_end);

                if (last->is_linear()) {
                    // This can only be a shifted part of g, we ignore it and move on
                    assert(static_cast<const LinearFunction &>(*last).d == g->d);
                    to_insert_end = std::prev(solutions_end);
                } else {
                    assert(last->is_hyperbolic());
                    // This can only be the shifted function of f, we are finished with g
                    assert(static_cast<const HyperbolicFunction &>(*last).a ==
                           static_cast<const HyperbolicFunction &>(*sub_f).a);
                    to_insert_end = solutions_end;
                }
            }

            assert(std::all_of(solutions.begin(), to_insert_end, [](const auto &s) {
                auto & [ d, h ] = s;
                return h->is_linear() || h.min_x > static_cast<const HyperbolicFunction &>(*h).b;
            }));

            return std::make_tuple(to_insert_end, solutions_end);
        },
        out);
}

template <typename OutIter>
auto combine_minimal(const PiecewieseDecHypOrLinFunction &f, const LimitedHypOrLinFunction &g,
                     OutIter out) {

    if (g->is_linear()) {
        return combine_minimal(f, static_cast<LimitedLinearFunction>(g), out);
    } else {
        assert(g->is_hyperbolic());
        return combine_minimal(f, static_cast<LimitedHyperbolicFunction>(g), out);
    }
}

inline auto combine_minimal(const LimitedLinearFunction &f, const LimitedLinearFunction &g) {
    Solution solution;
    combine_minimal(f, g, &solution[0]);
    return solution;
}

inline auto combine_minimal(const LimitedHyperbolicFunction &f, const LimitedLinearFunction &g) {
    Solution solution;
    combine_minimal(f, g, &solution[0]);
    return solution;
}

inline auto combine_minimal(const LimitedLinearFunction &f, const LimitedHyperbolicFunction &g) {
    Solution solution;
    combine_minimal(f, g, &solution[0]);
    return solution;
}

inline auto combine_minimal(const LimitedHyperbolicFunction &f,
                            const LimitedHyperbolicFunction &g) {
    Solution solution;
    combine_minimal(f, g, &solution[0]);
    return solution;
}

inline auto combine_minimal(const LimitedHypOrLinFunction &f, const LimitedHypOrLinFunction &g) {
    Solution solution;
    combine_minimal(f, g, &solution[0]);
    return solution;
}

inline auto combine_minimal(const PiecewieseDecHypOrLinFunction &f,
                            const LimitedHypOrLinFunction &g) {
    PiecewieseSolution solution;
    combine_minimal(
        f, g, make_sink_iter([&](auto &&partial_solution) {
            auto & [ pwf_delta, pwf_h ] = solution;
            auto && [ delta, h ] = partial_solution;

            assert(pwf_h.functions.empty() || !epsilon_less(pwf_h.functions.back().max_x, h.min_x));

            // we need to fix up rounding errors introduced by shifting
            if (!pwf_h.functions.empty() && pwf_h.functions.back().max_x != h.min_x) {
                h.min_x = pwf_h.functions.back().max_x;
            }
            assert(h->is_linear() || static_cast<const HyperbolicFunction &>(*h).b < h.min_x);
            pwf_delta.push_back(
                LimitedLinearFunction{h.min_x, h.max_x, std::move(delta)});
            pwf_h.functions.push_back(std::move(h));
        }));

    const auto & [ pwf_delta, pwf_h ] = solution;
    // assert(is_convex(pwf_h.functions.begin(), pwf_h.functions.end()));
    return solution;
}
}
#endif
