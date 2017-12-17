#ifndef CHARGE_COMMON_INTERPOLATING_FUNCTION_HPP
#define CHARGE_COMMON_INTERPOLATING_FUNCTION_HPP

#include "common/constant_function.hpp"
#include "common/function_traits.hpp"
#include "common/limited_function.hpp"
#include "common/linear_function.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>

namespace charge::common {

struct interpolate_linear final {};

template <typename MinBoundT, typename MaxBoundT, typename MonoticityT = not_monotone,
          typename InterpolationT = interpolate_linear>
struct InterpolatingFunction {
    // Compability to PiecewiseFunction
    InterpolatingFunction(
        std::initializer_list<LimitedFunction<LinearFunction, MinBoundT, MaxBoundT>>
            functions) noexcept {
        values.reserve(functions.size() + 1);
        for (const auto &f : functions) {
            push_back(f);
        }
    }

    InterpolatingFunction(std::initializer_list<std::tuple<double, double>> values_) noexcept
        : values{values_} {}

    InterpolatingFunction() noexcept = default;
    InterpolatingFunction(const InterpolatingFunction &other) noexcept { values = other.values; }
    InterpolatingFunction(InterpolatingFunction &&other) noexcept {
        values = std::move(other.values);
    }
    InterpolatingFunction &operator=(const InterpolatingFunction &other) noexcept {
        values = other.values;
        return *this;
    }
    InterpolatingFunction &operator=(InterpolatingFunction &&other) noexcept {
        values = std::move(other.values);
        return *this;
    };

    double operator()(double x) const noexcept {
        // first first x that is bigger then x
        auto rhs = std::find_if(values.begin(), values.end(),
                                [&](const auto &v) { return std::get<0>(v) > x; });

        if
            constexpr(std::is_same_v<MinBoundT, inf_bound>) {
                if (rhs == values.begin()) {
                    return std::numeric_limits<double>::infinity();
                }
            }
        if
            constexpr(std::is_same_v<MinBoundT, clamp_bound>) {
                if (rhs == values.begin()) {
                    return std::get<1>(values.front());
                }
            }
        if
            constexpr(std::is_same_v<MaxBoundT, inf_bound>) {
                if (rhs == values.end()) {
                    return std::numeric_limits<double>::infinity();
                }
            }
        if
            constexpr(std::is_same_v<MaxBoundT, clamp_bound>) {
                if (rhs == values.end()) {
                    return std::get<1>(values.back());
                }
            }
        assert(rhs != values.end());
        assert(rhs != values.begin());

        // no other implementation
        // the template parameter is just there in case we could need
        // step-wise functions
        static_assert(std::is_same_v<InterpolationT, interpolate_linear>);

        auto lhs = std::prev(rhs);

        assert(lhs != values.end());
        assert(rhs != values.end());

        auto[lhs_x, lhs_y] = *lhs;
        auto[rhs_x, rhs_y] = *rhs;

        auto alpha = (x - lhs_x) / (rhs_x - lhs_x);
        auto y = lhs_y + (rhs_y - lhs_y) * alpha;
        return y;
    }

    void push_back(const LimitedFunction<LinearFunction, MinBoundT, MaxBoundT> &f) {
        auto y_1 = f(f.min_x, no_bounds_checks{});
        auto y_2 = f(f.max_x, no_bounds_checks{});
        if (values.empty()) {
            values.emplace_back(f.min_x, y_1);
        }

        if (std::fabs(std::get<1>(values.back()) - y_2) > 0.0001) {
            values.emplace_back(f.max_x, y_2);
        }
    }

    void reserve(const std::size_t num_segments) { values.reserve(num_segments + 1); }

    void shrink_to_fit() { values.shrink_to_fit(); }

    auto size() const { return values.size(); }

    auto capacity() const { return values.capacity(); }

    auto cbegin() const { return values.cbegin(); }
    auto cend() const { return values.cend(); }
    auto begin() const { return values.begin(); }
    auto end() const { return values.end(); }
    auto begin() { return values.begin(); }
    auto end() { return values.end(); }

    LimitedFunction<LinearFunction, MinBoundT, MaxBoundT>
    limited(const std::size_t segment_index) const noexcept {
        assert(!values.empty());
        assert(segment_index < values.size());
        auto[x_1, y_1] = values[segment_index];

        if (values.size() == 1) {
            return LimitedFunction<LinearFunction, MinBoundT, MaxBoundT>{x_1, x_1,
                                                                         ConstantFunction{y_1}};
        }
        assert(values.size() >= 2);

        auto[x_2, y_2] = values[segment_index + 1];
        const auto slope = (y_2 - y_1) / (x_2 - x_1);
        return LimitedFunction<LinearFunction, MinBoundT, MaxBoundT>{
            x_1, x_2, LinearFunction{slope, x_1, y_1}};
    }

    void limit_from_x(double min_x_, double max_x_) {
        auto first_in = std::find_if(values.begin(), values.end(),
                                     [&](const auto &v) { return std::get<0>(v) > min_x_; });
        auto first_out = std::find_if(first_in, values.end(),
                                      [&](const auto &v) { return std::get<0>(v) > max_x_; });

        auto out = values.begin();
        if (first_in != values.begin()) {
            auto min_y = operator()(min_x_);
            *out++ = {min_x_, min_y};
        }
        out = std::copy(first_in, first_out, out);
        if (first_out != values.end()) {
            auto max_y = operator()(max_x_);
            assert(out != values.end());
            *out++ = {max_x_, max_y};
        }

        values.resize(out - values.begin());
    }

    void shift(double offset_x) {
        for (auto& v : values)
        {
            auto& [x, y] = v;
            x += offset_x;
        }
    }

    double min_x() const { return std::get<0>(values.front()); }

    double max_x() const { return std::get<0>(values.back()); }

  private:
    std::vector<std::tuple<double, double>> values;
};
}

#endif
