#ifndef CHARGE_HYP_LIN_FUNCTION_HPP
#define CHARGE_HYP_LIN_FUNCTION_HPP

#include "common/hyperbolic_function.hpp"
#include "common/linear_function.hpp"

#include <cassert>
#include <utility>

namespace charge {
namespace common {

// Encodes the inverse of a hyperbolic or linear function.
// We need this for HypOrLinFunction as the encapsulation of the inverse.
struct InvHypOrLinFunction {
    // a and d have dijunct ranges (a > 0 and d < 0)
    // so we can detect which one is used here by checking the sign
    inline bool is_linear() const { return lin.d <= 0.f; }
    inline bool is_hyperbolic() const { return !is_linear(); }

    InvHypOrLinFunction() noexcept = default;

    InvHypOrLinFunction(InverseHyperbolicFunction inv_hyp_) noexcept
        : inv_hyp(std::move(inv_hyp_)) {
        assert(is_hyperbolic());
    }

    InvHypOrLinFunction(LinearFunction lin_) noexcept : lin(std::move(lin_)) {
        assert(is_linear());
    }

    explicit operator const LinearFunction &() const {
        assert(is_linear());
        return lin;
    }

    explicit operator const InverseHyperbolicFunction &() const {
        assert(is_hyperbolic());
        return inv_hyp;
    }

    explicit operator LinearFunction &() {
        assert(is_linear());
        return lin;
    }

    explicit operator InverseHyperbolicFunction &() {
        assert(is_hyperbolic());
        return inv_hyp;
    }

    double operator()(double x) const {
        if (is_linear()) {
            return lin(x);
        } else {
            assert(is_hyperbolic());
            return inv_hyp(x);
        }
    }

    bool operator<(const InvHypOrLinFunction &other) const { return inv_hyp < other.inv_hyp; }

    bool operator==(const InvHypOrLinFunction &other) const { return inv_hyp == other.inv_hyp; }

    union {
        InverseHyperbolicFunction inv_hyp;
        LinearFunction lin;
    };
};

// Variant type for encoding
// hypbolic: a/(x-b)**2 + c
// linear: d(x-b) + c
// Using the following constraint: a > 0 and d <= 0.
struct HypOrLinFunction {
    // a and d have dijunct ranges (a > 0 and d < 0)
    // so we can detect which one is used here by checking the sign
    inline bool is_linear() const { return lin.d <= 0.f; }
    inline bool is_hyperbolic() const { return !is_linear(); }

    HypOrLinFunction() noexcept = default;

    HypOrLinFunction(HyperbolicFunction hyp_) noexcept : hyp(std::move(hyp_)) {
        assert(is_hyperbolic());
    }

    HypOrLinFunction(LinearFunction lin_) noexcept : lin(std::move(lin_)) { assert(is_linear()); }

    explicit operator const LinearFunction &() const noexcept {
        assert(is_linear());
        return lin;
    }

    explicit operator const HyperbolicFunction &() const noexcept {
        assert(is_hyperbolic());
        return hyp;
    }

    explicit operator LinearFunction &() noexcept {
        assert(is_linear());
        return lin;
    }

    explicit operator HyperbolicFunction &() noexcept {
        assert(is_hyperbolic());
        return hyp;
    }

    double operator()(double x) const noexcept {
        if (is_linear()) {
            return lin(x);
        } else {
            assert(is_hyperbolic());
            return hyp(x);
        }
    }

    InvHypOrLinFunction inverse() const noexcept {
        if (is_linear()) {
            return lin.inverse();
        } else {
            assert(is_hyperbolic());
            return hyp.inverse();
        }
    }

    double inverse(const double x) const noexcept {
        if (is_linear()) {
            return lin.inverse(x);
        } else {
            assert(is_hyperbolic());
            return hyp.inverse(x);
        }
    }

    double deriv(const double x) const noexcept {
        if (is_linear()) {
            return lin.deriv(x);
        } else {
            assert(is_hyperbolic());
            return hyp.deriv(x);
        }
    }

    void shift(const double x) noexcept {
        assert(lin.b == hyp.b);
        lin.b += x; // thanks to the union this changes hyp and lin
        assert(lin.b == hyp.b);
    }

    // offsets a function by y to the top
    void offset(double y) noexcept {
        assert(lin.c == hyp.c);
        lin.c += y; // thanks to the union this changes hyp and lin
        assert(lin.c == hyp.c);
    }

    bool operator<(const HypOrLinFunction &other) const { return hyp < other.hyp; }

    bool operator==(const HypOrLinFunction &other) const { return hyp == other.hyp; }

  private:
    union {
        LinearFunction lin;
        HyperbolicFunction hyp;
    };
};

template <> inline bool is_monotone_decreasing(const HypOrLinFunction &f) {
    if (f.is_linear()) {
        return is_monotone_decreasing(static_cast<const LinearFunction &>(f));
    } else {
        return is_monotone_decreasing(static_cast<const HyperbolicFunction &>(f));
    }
}

template <> struct is_monotone<HypOrLinFunction> : std::true_type {};
}
}

#endif
