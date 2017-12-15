#ifndef CHARGE_EV_CHARGING_FUNCION_CONTAINER_HPP
#define CHARGE_EV_CHARGING_FUNCION_CONTAINER_HPP

#include "ev/charging_function.hpp"
#include "ev/charging_model.hpp"
#include "ev/graph.hpp"

#include <vector>

namespace charge::ev {
class ChargingFunctionContainer {
  public:
    using node_id_t = typename TradeoffGraph::node_id_t;
    using weight_t = ChargingFunction;

    ChargingFunctionContainer(const std::vector<double> &charging_rates,
                              ChargingModel charging_model_)
        : charging_model(std::move(charging_model_)) {
        chargers.resize(charging_rates.size(), 0);
        for (auto node : common::irange<std::size_t>(0, charging_rates.size())) {
            chargers[node] = charging_model.get_index(charging_rates[node]);
        }
    }
    ChargingFunctionContainer(const ChargingFunctionContainer&) = default;
    ChargingFunctionContainer(ChargingFunctionContainer&&) = default;
    ChargingFunctionContainer& operator=(const ChargingFunctionContainer&) = default;
    ChargingFunctionContainer& operator=(ChargingFunctionContainer&&) = default;

    auto get_min_chargin_rate(const double charging_penalty) const {
        return charging_model.get_min_chargin_rate(charging_penalty);
    }

    auto get_max_chargin_rate(const double charging_penalty) const {
        return charging_model.get_max_chargin_rate(charging_penalty);
    }

    bool weighted(const node_id_t node) const {
        auto index = chargers[node];
        return index != 0;
    }

    const weight_t &weight(const node_id_t node) const {
        auto index = chargers[node];
        assert(index != 0);
        const auto &charging_function = charging_model[index];
        return charging_function;
    }

  private:
    std::vector<std::size_t> chargers;
    ChargingModel charging_model;
};
} // namespace charge::ev

#endif
