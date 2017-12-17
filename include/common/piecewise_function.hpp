#ifndef CHARGE_COMMON_PIECEWEISE_FUNCTION_HPP
#define CHARGE_COMMON_PIECEWEISE_FUNCTION_HPP

#include "common/limited_function.hpp"
#include "common/constant_function.hpp"

#include <algorithm>
#include <cassert>
#include <vector>
#include <limits>

namespace charge::common {

namespace detail {
template <typename Iter, typename OutIter>
auto clip(const Iter begin, const Iter end, double min_x, OutIter out) {
    std::for_each(begin, end, [min_x, &out](const auto &sub) {
        if (sub.max_x > min_x) {
            *out++ = {std::max(min_x, sub.min_x), sub.max_x, sub.function};
        }
    });
    return out;
}
}

template <typename FunctionT, typename MinBoundT, typename MaxBoundT,
          typename Monoticity = not_monotone>
struct PiecewieseFunction {
    using SubFunctionT = LimitedFunction<FunctionT, MinBoundT, MaxBoundT>;

    double operator()(const double x) const {
        const auto &sub_function = sub(x);
        return sub_function(x);
    }

    PiecewieseFunction() noexcept = default;
    PiecewieseFunction(const PiecewieseFunction &other) noexcept { functions = other.functions; };
    PiecewieseFunction(PiecewieseFunction &&other) noexcept {
        functions = std::move(other.functions);
    };
    PiecewieseFunction &operator=(const PiecewieseFunction &other) noexcept {
        functions = other.functions;
        return *this;
    }
    PiecewieseFunction &operator=(PiecewieseFunction &&other) noexcept {
        functions = std::move(other.functions);
        return *this;
    }

    PiecewieseFunction(SubFunctionT sub) noexcept : functions({std::move(sub)}) {}

    PiecewieseFunction(std::vector<SubFunctionT> functions_) noexcept
        : functions(std::move(functions_)) {}

    const SubFunctionT &sub(const double x) const {
        assert(!functions.empty());
        assert(
            std::is_sorted(functions.begin(), functions.end(),
                           [](const auto &lhs, const auto &rhs) { return lhs.min_x < rhs.min_x; }));

        auto sub_iter = std::find_if(functions.begin(), functions.end(),
                                     [x](const auto &function) { return function.min_x > x; });

        if (sub_iter == functions.begin()) {
            return functions.front();
        } else {
            return *std::prev(sub_iter);
        }
    }

    auto inverse() const {
        // clang-format off
        if constexpr(common::is_monotone<FunctionT>::value && !std::is_same_v<Monoticity, not_monotone>) {
            using InverseFunctionT = decltype(std::declval<FunctionT>().inverse());
            std::vector<LimitedFunction<InverseFunctionT, MinBoundT, MaxBoundT>> inverse_functions(functions.size());
            std::transform(functions.begin(), functions.end(), inverse_functions.begin(),
                           [](const auto &function) { return function.inverse(); });
            if constexpr(std::is_same_v<Monoticity, monotone_decreasing>) {
                std::reverse(inverse_functions.begin(), inverse_functions.end());
            } else {
                static_assert(std::is_same_v<Monoticity, monotone_increasing>);
                /* do nothing, already correct */
            }
            return PiecewieseFunction<InverseFunctionT, MinBoundT, MaxBoundT>{std::move(inverse_functions)};
        } else {
            throw std::runtime_error("Trying to invert non-monotone functions");
        }
        // clang-format on
    }

    template <typename OutFnT = FunctionT>
    PiecewieseFunction<OutFnT, MinBoundT, MaxBoundT, Monoticity> clip(const double min_x) const {
        PiecewieseFunction<OutFnT, MinBoundT, MaxBoundT, Monoticity> clipped;

        detail::clip(functions.begin(), functions.end(), min_x,
                     std::back_inserter(clipped.functions));

        return clipped;
    }

    void shift(const double offset_x) {
        for (auto &sub : functions) {
            sub.shift(offset_x);
        }
    }

    double min_x() const { return functions.front().min_x; }
    double max_x() const { return functions.back().max_x; }

    double min_y() const { return functions.back()(functions.back().max_x); }
    double max_y() const { return functions.front()(functions.front().min_x); }

    double inverse(double y) const {
        // clang-format off
        if constexpr(std::is_same_v<Monoticity, monotone_decreasing>) {
            if (y > operator()(min_x()))
                return -std::numeric_limits<double>::infinity();
            if (y < operator()(max_x()))
                return std::numeric_limits<double>::infinity();

            for (const auto &f : functions)
            {
                // FIXME check if calling the inverse
                // here is too expernsive
                auto x = f->inverse(y);
                if (x >= f.min_x && x <= f.max_x)
                    return x;
            }

        }
        else
        {
            std::runtime_error("Can't invert a non-monotone function.");
        }
        // clang-format on
    }

    double inverse_deriv(double alpha) const {
        // clang-format off
        if constexpr(std::is_same_v<Monoticity, monotone_decreasing>) {
            if (alpha < functions.front()->deriv(min_x()))
                return -std::numeric_limits<double>::infinity();
            if (alpha > functions.back()->deriv(max_x()))
                return std::numeric_limits<double>::infinity();

            for (const auto &f : functions)
            {
                // FIXME check if calling the inverse deriv
                // here is too expernsive
                auto x = f.inverse_deriv(alpha);
                if (x <= f.min_x)
                    return f.min_x;
                else if (x <= f.max_x)
                    return x;
            }

            assert(false);
        }
        else
        {
            std::runtime_error("Can't invert a non-monotone function.");
        }
        // clang-format on
    }

    auto size() const { return functions.size(); }

    auto capacity() const { return functions.capacity(); }

    void reserve(const std::size_t num_segments) { functions.reserve(num_segments + 1); }

    void shrink_to_fit() { functions.shrink_to_fit(); }

    auto cbegin() const { return functions.cbegin(); }
    auto cend() const { return functions.cend(); }
    auto begin() const { return functions.begin(); }
    auto end() const { return functions.end(); }
    auto begin() { return functions.begin(); }
    auto end() { return functions.end(); }

    void limit_from_x(double min_x, double max_x) {
        auto new_end = std::remove_if(functions.begin(), functions.end(), [&](const auto &f) {
            return f.max_x < min_x || f.min_x > max_x;
        });
        functions.resize(new_end - functions.begin());
        if (functions.size() > 0)
        {
            functions.front().min_x = min_x;
            functions.back().max_x = max_x;
        }
    }

    void limit_from_y(double min_y, double max_y) {
        if (functions.empty())
            return;
        // clang-format off
        if constexpr(std::is_same_v<Monoticity, monotone_decreasing>) {
            if (this->max_y() <= min_y)
            {
                auto min_x = this->min_x();
                functions.clear();
                functions.push_back(SubFunctionT {min_x, min_x, ConstantFunction {min_y}});
                return;
            }

            // return the first segment where max_y > f(max_x)
            auto range_begin = std::lower_bound(functions.begin(), functions.end(), max_y, [](const auto &function, const double y) {
                return y <= function(function.max_x);
            });
            // return the first segment where min_y >= f(min_x)
            auto range_end = std::lower_bound(range_begin, functions.end(), min_y, [](const auto &function, const double y) {
                return y < function(function.min_x);
            });
            assert(std::distance(range_begin, range_end) >= 0);

            // clip first segment exact
            if (range_begin != range_end)
            {
                auto& first = *range_begin;
                first.limit_from_y(min_y, max_y);
                if (first.max_x < first.min_x)
                {
                    range_begin++;
                }
            }
            if (range_begin != range_end && std::prev(range_end) != range_begin)
            {
                auto& last = *std::prev(range_end);
                last.limit_from_y(min_y, max_y);
                if (last.max_x < last.min_x)
                {
                    range_end--;
                }
            }
            assert(std::distance(range_begin, range_end) >= 0);

            if (range_begin != functions.begin())
            {
                range_end = std::copy(range_begin, range_end, functions.begin());
            }

            if (range_end != functions.end())
            {
                functions.resize(std::distance(functions.begin(), range_end));
            }
        } else {
            throw std::runtime_error(
                "Limiting non-monotone decreasing functions is not implemented");
        }
        // clang-format on
    }

    bool operator==(const PiecewieseFunction &other) const {
        if (other.functions.size() != functions.size())
            return false;

        return functions == other.functions;
    }

    std::vector<SubFunctionT> functions;
};

template <typename FunctionT, typename MinBoundT, typename MaxBoundT>
bool is_monotone_decreasing(
    const PiecewieseFunction<FunctionT, MinBoundT, MaxBoundT, monotone_decreasing> &) {
    return true;
}

template <typename FunctionT, typename MinBoundT, typename MaxBoundT>
struct is_monotone<PiecewieseFunction<FunctionT, MinBoundT, MaxBoundT, monotone_decreasing>>
    : std::true_type {};

template <typename FunctionT, typename MinBoundT, typename MaxBoundT>
struct is_monotone<PiecewieseFunction<FunctionT, MinBoundT, MaxBoundT, monotone_increasing>>
    : std::true_type {};
}

#endif
