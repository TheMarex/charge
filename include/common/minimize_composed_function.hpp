#ifndef CHARGE_COMMON_MINIMIZE_COMPOSED_FUNCTION_HPP
#define CHARGE_COMMON_MINIMIZE_COMPOSED_FUNCTION_HPP

#include "common/compose_functions.hpp"
#include "common/linear_function.hpp"
#include "common/minimize_combined_function.hpp"
#include "common/piecewise_function.hpp"
#include "common/stateful_function.hpp"

namespace charge::common {

// Computes h(x) = min_d g(x - d, f(d)) (d < x) for g(x, y) being a piece-wise linear function for
// fixed y and f either a hyperbolic or linear function.
// We assume that g has the form: g(x, y) = Capacity - cf(x + cf^-1(Capacity - y))
//
// There are two possible solutions:
// 1. delta(x) = x: The function is minimized g(0, f(d)) (no time spend on g)
// 2. delta(x) = f_min_x (time on f is)

namespace detail {

inline auto merge_solutions(const std::vector<DeltaAndOptimalFunction> &results) {
    const auto transform = [](const DeltaAndOptimalFunction &s) -> const LimitedHypOrLinFunction & {
        const auto & [ delta, function ] = s;
        return function;
    };

    auto begin = make_adapter_iter(results.begin(), transform);
    auto end = make_adapter_iter(results.end(), transform);

    PiecewieseSolution solution;
    auto & [ delta, function ] = solution;
    std::vector<std::uint32_t> indices;

    detail::lower_envelop(begin, end, std::back_inserter(function.functions),
                          std::back_inserter(indices));

    auto function_iter = function.functions.begin();
    assert(function.functions.size() == indices.size());
    delta.reserve(indices.size());
    std::for_each(indices.begin(), indices.end(), [&](const auto index) {
        auto sub_delta = LimitedLinearFunction{function_iter->min_x, function_iter->max_x,
                                               std::get<0>(results[index])};
        function_iter++;
        std::get<0>(solution).push_back(sub_delta);
    });

    return solution;
}

// Composes a single fragement of f and g
template <typename OutIter>
auto compose_minimal(const LimitedLinearFunction &sub_f, const LimitedLinearFunction &sub_g,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {
    // only consider the subfunction
    // if spending time on it could be better
    // g' <= f'
    if (sub_g->d > sub_f->d)
        return out;

    auto min_y = sub_g(sub_g.max_x);
    auto max_y = sub_g(sub_g.min_x);

    assert(sub_g->d < 0);
    // special case handling for constant functions
    if (sub_f->d == 0) {
        assert(sub_f.min_x == sub_f.max_x);
        auto y = sub_f(sub_f.min_x);

        if (min_y < y) {
            ConstantFunction delta{sub_f.min_x};
            auto h = compose_function(delta, *sub_f, g);
            if (!h.functions.empty() && epsilon_less(h.min_x(), h.max_x())) {
                *out++ = PiecewieseSolution{
                    InterpolatingIncFunction{{{h.min_x(), h.max_x(), delta}}}, std::move(h)};
            }
        }

        return out;
    }
    assert(sub_f->d < 0);

    // intersect image of sub_f with y-domain of g^-1
    auto min_x = std::max(sub_f.min_x, sub_f->inverse(max_y));
    auto max_x = std::min(sub_f.max_x, sub_f->inverse(min_y));

    auto delta_candidate = min_x;

    // For all x in [min_x, max_x] we know that:
    // g'(g^-1(f(x))) = f'(x)
    // so we can chose min_x
    if (min_x <= max_x) {
        ConstantFunction delta{delta_candidate};
        auto h = compose_function(delta, *sub_f, g);
        if (!h.functions.empty() && epsilon_less(h.min_x(), h.max_x())) {
            *out++ = PiecewieseSolution{InterpolatingIncFunction{{{h.min_x(), h.max_x(), delta}}},
                                        std::move(h)};
        }
    }

    return out;
}

// Composes a single fragment of a stateful function with a hyperbolic function
template <typename OutIter>
auto compose_minimal(const LimitedHyperbolicFunction &sub_f, const LimitedLinearFunction &sub_g,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {
    // Compute the position on f where f has the same slope as sub
    // sub->d = f'(delata_candidate) and
    // sub->d < f'(d) for d > delta_candidate (slope of f is monotone decreasing)
    auto delta_candidate = sub_f->b + std::cbrt((-2 * sub_f->a) / sub_g->d);
    if (delta_candidate < sub_f.min_x || delta_candidate > sub_f.max_x)
        return out;

    // at which x coordinate would sub_g have a consumption
    // of f(x_candiate)
    auto y_candiate = sub_f(delta_candidate);
    auto min_y = sub_g(sub_g.max_x);
    auto max_y = sub_g(sub_g.min_x);

    // if this is true we know that
    // sub->d = g'(x - delta_candidate, f(delta_candidate)) <= f'(x) (for x >=
    // delta_candidate)
    if (min_y <= y_candiate && max_y >= y_candiate) {
        ConstantFunction delta{delta_candidate};
        auto h = compose_function(delta, *sub_f, g);
        if (!h.functions.empty() && epsilon_less(h.min_x(), h.max_x())) {
            *out++ = PiecewieseSolution{InterpolatingIncFunction{{{h.min_x(), h.max_x(), delta}}},
                                        std::move(h)};
        }
    }

    return out;
}

template <typename OutIter>
auto compose_minimal(const LimitedHypOrLinFunction &sub_f, const LimitedLinearFunction &sub_g,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {
    if (sub_f->is_linear()) {
        return compose_minimal(static_cast<LimitedLinearFunction>(sub_f), sub_g, g, out);
    } else {
        assert(sub_f->is_hyperbolic());
        return compose_minimal(static_cast<LimitedHyperbolicFunction>(sub_f), sub_g, g, out);
    }
}

// Returns the result as single limited fragments
// not as piecewise functions.
template <typename OutIter>
auto compose_minimal(const LimitedLinearFunction &sub_f,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {
    auto max_y = sub_f(sub_f.min_x);

    auto x_start = g.inv_function(max_y);
    const auto &sub_g = g.function.sub(x_start);

    compose_minimal(sub_f, sub_g, g, make_sink_iter([&](PiecewieseSolution &&solution) {
                        auto && [ delta, h ] = solution;
                        for (auto &&sub_h : h.functions) {
                            *out++ = DeltaAndOptimalFunction{*delta.limited(0), std::move(sub_h)};
                        }
                    }));
    return out;
}

template <typename OutIter>
auto compose_minimal(const LimitedHyperbolicFunction &sub_f,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {

    auto f_min_deriv = sub_f->deriv(sub_f.min_x);
    auto f_max_deriv = sub_f->deriv(sub_f.max_x);

    auto begin =
        std::lower_bound(g.function.functions.begin(), g.function.functions.end(), f_min_deriv,
                         [](const auto &sub_g, const double deriv) { return sub_g->d < deriv; });

    auto end =
        std::upper_bound(begin, g.function.functions.end(), f_max_deriv,
                         [](const double deriv, const auto &sub_g) { return deriv < sub_g->d; });

    std::for_each(begin, end, [&](const auto &sub_g) {
        compose_minimal(
            sub_f, sub_g, g, make_sink_iter([&](PiecewieseSolution &&solution) {
                auto && [ delta, h ] = solution;
                for (auto &&sub_h : h.functions) {
                    *out++ = DeltaAndOptimalFunction{*delta.limited(0), std::move(sub_h)};
                }
            }));
    });

    return out;
}

template <typename OutIter>
auto compose_minimal(const LimitedHypOrLinFunction &f, const StatefulPiecewieseDecLinearFunction &g,
                     OutIter out) {
    if (f->is_linear()) {
        return compose_minimal(static_cast<LimitedLinearFunction>(f), g, out);
    } else {
        assert(f->is_hyperbolic());
        return compose_minimal(static_cast<LimitedHyperbolicFunction>(f), g, out);
    }
}
} // namespace detail

// Returns a single (non-convex) piecewise function
inline auto compose_minimal(const LimitedLinearFunction &f,
                            const StatefulPiecewieseDecLinearFunction &g) {
    std::vector<DeltaAndOptimalFunction> result;

    detail::compose_minimal(f, g, std::back_inserter(result));

    return detail::merge_solutions(result);
}

// Returns a single (non-convex) piecewise function
inline auto compose_minimal(const LimitedHyperbolicFunction &f,
                            const StatefulPiecewieseDecLinearFunction &g) {
    std::vector<DeltaAndOptimalFunction> result;

    detail::compose_minimal(f, g, std::back_inserter(result));

    return detail::merge_solutions(result);
}

// Returns a single (non-convex) piecewise function
inline auto compose_minimal(const LimitedHypOrLinFunction &f,
                            const StatefulPiecewieseDecLinearFunction &g) {
    if (f->is_linear()) {
        return compose_minimal(static_cast<LimitedLinearFunction>(f), g);
    } else {
        assert(f->is_hyperbolic());
        return compose_minimal(static_cast<LimitedHyperbolicFunction>(f), g);
    }
}

// Returns a single (non-convex) piecewise function
inline auto compose_minimal(const PiecewieseDecHypOrLinFunction &f,
                            const StatefulPiecewieseDecLinearFunction &g) {

    std::vector<DeltaAndOptimalFunction> results;

    for (const auto &sub_f : f.functions) {
        detail::compose_minimal(sub_f, g, std::back_inserter(results));
    }

    return detail::merge_solutions(results);
}

// Returns multiple convex functions, that potentially dominate each other
template <typename OutIter>
auto compose_minimal(const PiecewieseDecHypOrLinFunction &f,
                     const StatefulPiecewieseDecLinearFunction &g, OutIter out) {
    if (f.size() == 1 && f.functions.front()->is_linear()) {
        const auto lin = static_cast<const LimitedLinearFunction>(f.functions.front());
        if (lin->d == 0) {
            auto g_x = g.inv_function(lin->c);
            if (g_x >= g.function.min_x() && g_x <= g.function.max_x())
            {
                out = detail::compose_minimal(lin, g.function.sub(g_x), g, out);
            }
            return out;
        }
    }

    auto f_begin = f.functions.begin();
    auto g_begin = g.function.functions.begin();

    auto f_end = f.functions.end();
    auto g_end = g.function.functions.end();

    auto f_iter = f_begin;
    auto g_iter = g_begin;

    auto f_min_left_deriv = -std::numeric_limits<double>::infinity();

    while (f_iter != f_end && g_iter != g_end) {
        const auto &sub_f = *f_iter;
        auto g_deriv = g_iter->function.d;
        auto f_min_right_deriv = sub_f->deriv(sub_f.min_x);

        assert(g_iter != g_end);
        while (g_deriv < f_min_right_deriv) {
            // there is a critical point on f at f_min
            if (g_deriv > f_min_left_deriv) {
                auto f_y = sub_f(sub_f.min_x);
                auto g_x = g.inv_function(f_y);
                if (g_x >= g_iter->min_x && g_x < g_iter->max_x) {
                    auto corner_f =
                        LimitedLinearFunction{sub_f.min_x, sub_f.min_x, ConstantFunction{f_y}};
                    out = detail::compose_minimal(corner_f, *g_iter, g, out);
                }
            }

            g_iter++;
            if (g_iter == g_end)
                break;
            g_deriv = g_iter->function.d;
        }
        if (g_iter == g_end)
            break;
        assert(g_iter != g_end);
        assert(g_deriv >= f_min_right_deriv);

        const auto &sub_g = *g_iter;
        auto f_max_left_deriv = f_iter->function.deriv(f_iter->max_x);

        if (sub_g->d <= f_max_left_deriv) {
            assert(g_deriv >= f_min_right_deriv);
            assert(g_deriv <= f_max_left_deriv);

            // We know f'(min_x) <= d <= f'(max_x)
            // so there must be f'(x_start) = d.
            out = detail::compose_minimal(sub_f, sub_g, g, out);
            g_iter++;
        } else {
            f_min_left_deriv = f_max_left_deriv;
            f_iter++;
        }
    }

    // Check f.max_x for a critical point
    if (g_iter != g_end) {
        auto f_min_right_deriv = 0;
        auto g_deriv = g_iter->function.d;
        while (g_deriv < f_min_right_deriv) {
            // there is a critical point on f at f_min
            if (g_deriv > f_min_left_deriv) {
                auto f_min_x = f.functions.back().max_x;
                auto f_y = f(f.functions.back().max_x);
                auto g_x = g.inv_function(f_y);
                if (g_x >= g_iter->min_x && g_x < g_iter->max_x) {
                    auto corner_f = LimitedLinearFunction{f_min_x, f_min_x, ConstantFunction{f_y}};
                    out = detail::compose_minimal(corner_f, *g_iter, g, out);
                }
            }

            g_iter++;
            if (g_iter == g_end)
                break;
            g_deriv = g_iter->function.d;
        }
    }

    return out;
}
} // namespace charge::common

#endif
