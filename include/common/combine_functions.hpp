#ifndef CHARGE_COMMON_COMBINE_FUNCTIONS_HPP
#define CHARGE_COMMON_COMBINE_FUNCTIONS_HPP

#include "common/constant_function.hpp"
#include "common/hyp_lin_function.hpp"
#include "common/hyperbolic_function.hpp"
#include "common/linear_function.hpp"
#include "common/shift_function.hpp"

#include <cassert>
#include <cmath>

namespace charge::common {

// These function overloads compute the inear combination of `f` and `g`:
// h(x) = f(delta(x)) + g(x - delta(x))
// but only work for specific delta functions that are of two forms:
//
// 1. Constant function
// 2. Linear function with slope 1 (unit function)
// 3. Only if f and g are hyperbolic: Linear function for which f'(delta(x)) =
// g'(x - delta(x))
//
// All other functions will result on undefined results.



//
// Hyperbolic + Hyperbolic
//

// h(x) = f(delta(x)) + g(x - delta(x)))
//      = f(delta.c) + g(x - delta.c)
inline HyperbolicFunction combine(const HyperbolicFunction &f, const HyperbolicFunction &g,
                                  const ConstantFunction &delta) noexcept {
    HyperbolicFunction h;

    h.a = g.a;
    h.b = g.b + delta.c;
    h.c = g.c + f(delta.c);

    return h;
}

// delta has to be of a specific form (optimal solution) for this to work.
// delta need to fulfill: f'(delta(x)) = g'(x - delta(x))
inline HyperbolicFunction combine(const HyperbolicFunction &f, const HyperbolicFunction &g,
                                  const LinearFunction &) noexcept {
    HyperbolicFunction h;

    h.a = f.a + g.a + 3 * (std::cbrt(f.a * f.a * g.a) + std::cbrt(f.a * g.a * g.a));
    h.b = f.b + g.b;
    h.c = f.c + g.c;

    return h;
}

//
// Linear + Linear
//

// h = f(c_d) + g(x - c_d)
inline LinearFunction combine(const LinearFunction &f, const LinearFunction &g,
                              const ConstantFunction &delta) noexcept {
    LinearFunction h;
    h.d = g.d;
    h.b = g.b + delta.c;
    h.c = g.c + f(delta.c);
    return h;
}

// We assume f and g have the same slope and delta interpolated between f_min_x and f_max_x
// h = f(d(x)) + g(x - d(x)) = d(x - b_1 - b_2) + c_1 + c_2
inline LinearFunction combine(const LinearFunction &f, const LinearFunction &g, const LinearFunction &) noexcept {
    assert(f.d == g.d);
    LinearFunction h;
    h.d = f.d;
    h.b = f.b + g.b;
    h.c = f.c + g.c;
    return h;
}

//
// Linear + Constant
//

// h = f(c_d) + g(x - c_b)
inline ConstantFunction combine(const LinearFunction &f, const ConstantFunction &g,
                                const ConstantFunction &delta) noexcept {
    ConstantFunction h;
    h.c = g.c + f(delta.c);
    return h;
}

// h = f(x - c_b) + g(c_b)
inline LinearFunction combine(const LinearFunction &f, const ConstantFunction &g,
                              const ShiftFunction &delta) noexcept {
    LinearFunction h;
    h.d = f.d;
    h.b = f.b + delta.b;
    h.c = f.c + g(delta.b);
    return h;
}

//
// Constant + Constant
//

inline ConstantFunction combine(const ConstantFunction &f, const ConstantFunction &g,
                                const ConstantFunction &) noexcept {
    ConstantFunction h;
    h.c = f.c + g.c;
    return h;
}

inline ConstantFunction combine(const ConstantFunction &f, const ConstantFunction &g,
                                const ShiftFunction &) noexcept {
    ConstantFunction h;
    h.c = f.c + g.c;
    return h;
}

//
// Hyperbolic + Linear
//

// h(x) = f(delta(x)) + g(x - delta(x))
//      = f(delta.c) + g.d * (x - delta.c - g.b) + g.c
//      = g.d * (x - (delta.c + g.b)) + g.c + f(delta.c)
inline LinearFunction combine(const HyperbolicFunction &f, const LinearFunction &g,
                              const ConstantFunction &delta) noexcept {
    LinearFunction lin;
    lin.d = g.d;
    lin.b = g.b + delta.c;
    lin.c = g.c + f(delta.c);

    return lin;
}

// h(x) = f(delta(x)) + g(x - delta(x))
//      = f(x - delta.c) + g(delta.c)
inline HyperbolicFunction combine(const HyperbolicFunction &f, const LinearFunction &g,
                                  const ShiftFunction &delta) noexcept {
    HyperbolicFunction hyp;
    hyp.a = f.a;
    hyp.b = f.b + delta.b;
    hyp.c = f.c + g(delta.b);

    return hyp;
}

// Hyperbolic + Constant

// h(x) = f(delta(x)) + g(x - delta(x))
//      = f(x - delta.b) + g.c
inline HyperbolicFunction combine(const HyperbolicFunction &f, const ConstantFunction &g,
                                  const ShiftFunction &delta) noexcept {
    HyperbolicFunction hyp;
    hyp.a = f.a;
    hyp.b = f.b + delta.b;
    hyp.c = f.c + g.c;

    return hyp;
}

// h(x) = f(delta(x)) + g(x - delta(x))
//      = f(delta.d) + g.c
inline ConstantFunction combine(const HyperbolicFunction &f, const ConstantFunction &g,
                                const ConstantFunction &delta) noexcept {
    ConstantFunction h;
    h.c = f(delta.c) + g.c;
    return h;
}

// HypOrLin and constant

inline HypOrLinFunction combine(const HypOrLinFunction &f, const ConstantFunction &g,
                                const ShiftFunction &delta) noexcept {
    if (f.is_linear()) {
        return combine(static_cast<const LinearFunction &>(f), g, delta);
    } else {
        assert(f.is_hyperbolic());
        return combine(static_cast<const HyperbolicFunction &>(f), g, delta);
    }
}

inline ConstantFunction combine(const HypOrLinFunction &f, const ConstantFunction &g,
                                const ConstantFunction &delta) noexcept {
    if (f.is_linear()) {
        return combine(static_cast<const LinearFunction &>(f), g, delta);
    } else {
        assert(f.is_hyperbolic());
        return combine(static_cast<const HyperbolicFunction &>(f), g, delta);
    }
}

// These template function encode the commutivity rule for combining functions
template<typename Fn1, typename Fn2>
inline auto combine(const Fn1& f, const Fn2 &g, const ShiftFunction &delta)
{
    return combine(g, f, ConstantFunction {delta.b});
}

template<typename Fn1, typename Fn2>
inline auto combine(const Fn1& f, const Fn2 &g, const ConstantFunction &delta)
{
    return combine(g, f, ShiftFunction {delta.c});
}


}

#endif
