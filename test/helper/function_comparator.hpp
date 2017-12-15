#include "common/hyp_lin_function.hpp"
#include "common/irange.hpp"
#include "common/limited_function.hpp"
#include "common/interpolating_function.hpp"

#include "function_printer.hpp"

#include <catch.hpp>

namespace detail {
template <typename Fn> class ApproxFunction;

template <> class ApproxFunction<charge::common::LinearFunction> {
  public:
    using value_t = charge::common::LinearFunction;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        return value.b == Approx(other.b) && value.c == Approx(other.c) &&
               value.d == Approx(other.d);
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};

template <> class ApproxFunction<charge::common::HyperbolicFunction> {
  public:
    using value_t = charge::common::HyperbolicFunction;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        return value.b == Approx(other.b) && value.c == Approx(other.c) &&
               value.a == Approx(other.a);
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};

template <> class ApproxFunction<charge::common::HypOrLinFunction> {
  public:
    using value_t = charge::common::HypOrLinFunction;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        if (value.is_linear() && other.is_linear()) {
            return ApproxFunction<charge::common::LinearFunction>(
                       static_cast<const charge::common::LinearFunction &>(other)) ==
                   static_cast<const charge::common::LinearFunction &>(value);
        } else if (value.is_hyperbolic() && other.is_hyperbolic()) {
            return ApproxFunction<charge::common::HyperbolicFunction>(
                       static_cast<const charge::common::HyperbolicFunction &>(other)) ==
                   static_cast<const charge::common::HyperbolicFunction &>(value);
        }
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};

template <typename FnT>
class ApproxFunction<
    charge::common::LimitedFunction<FnT, charge::common::inf_bound, charge::common::clamp_bound>> {
  public:
    using value_t = charge::common::LimitedFunction<FnT, charge::common::inf_bound,
                                                    charge::common::clamp_bound>;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        return value.min_x == Approx(other.min_x) && value.max_x == Approx(other.max_x) &&
               ApproxFunction<FnT>(other.function) == value.function;
    }
    bool operator!=(const value_t &other) const {
        return !(operator==(other));
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};

template <typename FnT>
class ApproxFunction<
    charge::common::PiecewieseFunction<FnT, charge::common::inf_bound, charge::common::clamp_bound,
                                       charge::common::monotone_decreasing>> {
  public:
    using value_t = charge::common::PiecewieseFunction<FnT, charge::common::inf_bound,
                                                       charge::common::clamp_bound,
                                                       charge::common::monotone_decreasing>;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        if (other.size() != value.size())
            return false;

        for (auto iter = value.begin(), other_iter = other.begin();
             iter != value.end() && other_iter != value.end();
             iter++, other_iter++) {
            if (ApproxFunction<charge::common::LimitedFunction<FnT, charge::common::inf_bound,
                                                               charge::common::clamp_bound>>(
                    *iter) != *other_iter)
                return false;
        }
        return true;
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};

template<typename MonoticityT>
class ApproxFunction<
    charge::common::InterpolatingFunction<charge::common::inf_bound, charge::common::clamp_bound,
                                       MonoticityT>> {
  public:
    using value_t = charge::common::InterpolatingFunction<charge::common::inf_bound,
                                                       charge::common::clamp_bound,
                                                       MonoticityT>;

    ApproxFunction(const value_t &value) : value(value) {}

    bool operator==(const value_t &other) const {
        if (other.size() != value.size())
            return false;

        for (auto iter = value.begin(), other_iter = other.begin();
             iter != value.end() && other_iter != value.end();
             iter++, other_iter++) {
             if (Approx(std::get<0>(*iter)) != std::get<0>(*other_iter))
                return false;
             if (Approx(std::get<1>(*iter)) != std::get<1>(*other_iter))
                return false;
        }
        return true;
    }

    std::string toString() const {
        return std::string("ApproxFunction( ") + Catch::toString(value) + std::string(" )");
    }

    const value_t &value;
};
}

template <typename FnT> auto ApproxFunction(const FnT &fn) {
    return detail::ApproxFunction<FnT>(fn);
}

class Samples {
    std::vector<double> xs;
    std::vector<double> ys;

    int error_index = -1;
    double error_value;

  public:
    template <typename OtherFunctionT>
    Samples(const OtherFunctionT &other, double min_x, double max_x, double resolution = 0.1) {
        std::size_t num_samples = (max_x - min_x) / resolution;

        for (auto index : charge::common::irange(0UL, num_samples)) {
            auto x = index * resolution;
            xs.push_back(x);
            ys.push_back(other(x));
        }
    }

    template <typename OtherFunctionT, typename Predicate>
    Samples(const std::vector<OtherFunctionT> &others, Predicate p, double min_x, double max_x,
            double resolution = 0.1) {
        std::size_t num_samples = (max_x - min_x) / resolution;

        for (auto index : charge::common::irange(0UL, num_samples)) {
            auto x = index * resolution;
            xs.push_back(x);
            auto y = std::numeric_limits<double>::infinity();
            for (const auto &other : others) {
                y = p(y, other(x));
            }
            ys.push_back(y);
        }
    }

    template <typename FunctionT> bool operator==(const FunctionT &function) {
        for (const auto index : charge::common::irange(0UL, xs.size())) {
            auto y = function(xs[index]);
            if (ys[index] != y) {
                error_index = index;
                error_value = y;
                return false;
            }
        }
        return true;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "Samples { [" << xs.front() << "," << xs.back() << "]";
        if (error_index >= 0) {
            ss << " error: [" << xs[error_index] << "] = " << ys[error_index]
               << " != " << error_value;
        }
        ss << " }";
        return ss.str();
    }
};

namespace Catch {
template <> inline std::string toString<Samples>(const Samples &f) { return f.toString(); }
template <>
inline std::string toString<detail::ApproxFunction<charge::common::LimitedLinearFunction>>(
    const detail::ApproxFunction<charge::common::LimitedLinearFunction> &f) {
    return f.toString();
}
template <>
inline std::string toString<detail::ApproxFunction<charge::common::LimitedHypOrLinFunction>>(
    const detail::ApproxFunction<charge::common::LimitedHypOrLinFunction> &f) {
    return f.toString();
}
template <>
inline std::string toString<detail::ApproxFunction<charge::common::LinearFunction>>(
    const detail::ApproxFunction<charge::common::LinearFunction> &f) {
    return f.toString();
}
template <>
inline std::string toString<detail::ApproxFunction<charge::common::HyperbolicFunction>>(
    const detail::ApproxFunction<charge::common::HyperbolicFunction> &f) {
    return f.toString();
}
template <>
inline std::string toString<detail::ApproxFunction<charge::common::HypOrLinFunction>>(
    const detail::ApproxFunction<charge::common::HypOrLinFunction> &f) {
    return f.toString();
}
}
