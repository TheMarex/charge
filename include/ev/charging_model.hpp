#ifndef CHARGE_CHARGING_MODEL_HPP
#define CHARGE_CHARGING_MODEL_HPP

#include "common/irange.hpp"

#include "ev/charging_function.hpp"

#include <cmath>
#include <unordered_map>
#include <vector>

namespace charge::ev {

namespace detail {

template <typename RateProfileT, typename CapacityProfileT>
auto make_charging_function(const RateProfileT &rate_profile,
                            const CapacityProfileT &capacity_profile, const double rate,
                            const double capacity) {

    common::PiecewieseFunction<common::LinearFunction, common::inf_bound, common::clamp_bound,
                               common::monotone_decreasing>
        base_cf;

    double last_x = 0;
    double last_y = capacity;
    for (auto index : common::irange<std::size_t>(0, rate_profile.size())) {
        auto dc = (capacity_profile[index] - capacity_profile[index + 1]) * capacity;
        auto dt = -dc * 60 * 60 / (rate_profile[index] * rate);
        double slope = dc / dt;
        assert(slope < 0);
        double next_x = last_x + dt;
        double next_y = last_y + dc;

        common::LimitedFunction<common::LinearFunction, common::inf_bound, common::clamp_bound>
            function{last_x, next_x, common::LinearFunction{slope, last_x, last_y}};

        base_cf.functions.emplace_back(std::move(function));
        last_x = next_x;
        last_y = next_y;
    }

    return ChargingFunction{std::move(base_cf)};
}

} // namespace detail

class ChargingModel {
  public:
    ChargingModel(const double capacity) : capacity(capacity) {}
    ChargingModel(const ChargingModel&) = default;
    ChargingModel(ChargingModel&&) = default;
    ChargingModel& operator=(const ChargingModel&) = default;
    ChargingModel& operator=(ChargingModel&&) = default;

    std::size_t get_index(const double rate) {
        if (rate == 0)
            return 0UL;

        if (auto iter = rate_to_idx.find(rate); iter != rate_to_idx.end()) {
            return iter->second;
        } else {
            auto rate_idx = charging_functions.size() + 1;
            rate_to_idx[rate] = rate_idx;
            charging_functions.push_back(make_charging_function(rate));
            return rate_idx;
        }
    }

    std::size_t get_index(const double rate) const {
        if (rate == 0)
            return 0UL;

        if (auto iter = rate_to_idx.find(rate); iter != rate_to_idx.end()) {
            return iter->second;
        } else {
            throw std::runtime_error("No charging function found for rate " + std::to_string(rate));
        }
    }

    auto &operator[](const std::size_t rate_idx) {
        assert(rate_idx > 0);
        return charging_functions[rate_idx - 1];
    }

    const auto &operator[](const std::size_t rate_idx) const {
        assert(rate_idx > 0);
        return charging_functions[rate_idx - 1];
    }

    double get_capacity() const { return capacity; }

    double get_min_chargin_rate(const double charging_penalty) const {
        auto min_charging_rate = std::numeric_limits<double>::infinity();
        for (const auto &fn : charging_functions) {
            auto dt = fn.function.functions.front().max_x - fn.function.functions.front().min_x + charging_penalty;
            auto dy = fn.function(fn.function.functions.front().max_x) - fn.function(fn.function.functions.front().min_x);
            auto rate = dy / dt;
            min_charging_rate = std::min(rate, min_charging_rate);
        }
        return min_charging_rate;
    }

    double get_max_chargin_rate(const double charging_penalty) const {
        auto max_charging_rate = -std::numeric_limits<double>::infinity();
        for (const auto &fn : charging_functions) {
            auto dt = fn.function.functions.front().max_x - fn.function.functions.front().min_x + charging_penalty;
            auto dy = fn.function(fn.function.functions.front().max_x) - fn.function(fn.function.functions.front().min_x);
            auto rate = dy / dt;
            max_charging_rate = std::max(rate, max_charging_rate);
        }
        return max_charging_rate;
    }

  private:
    ChargingFunction make_charging_function(const double rate) const {
        // fast-charger
        if (rate > 40000) {
            return detail::make_charging_function(quick_rate_profile, quick_capacity_profile, rate,
                                                  capacity);
        }
        // normal charging profile
        else {
            return detail::make_charging_function(full_rate_profile, full_capacity_profile, rate,
                                                  capacity);
        }
    }

    double capacity;
    std::vector<ChargingFunction> charging_functions;
    std::unordered_map<double, std::size_t> rate_to_idx;

    std::array<double, 5> full_rate_profile = {
        {0.99208922, 0.86715031, 0.63569885, 0.43195935, 0.1457976}};
    std::array<double, 6> full_capacity_profile = {{0.0, 0.8, 0.85, 0.9, 0.95, 1.0}};
    std::array<double, 1> quick_rate_profile = {{1.0}};
    std::array<double, 2> quick_capacity_profile = {{0, 0.8}};
};
} // namespace charge::ev

#endif
