#ifndef CHARGE_SERVER_HANDLERS_FPC_DIJKSTRA_HPP
#define CHARGE_SERVER_HANDLERS_FPC_DIJKSTRA_HPP

#include "server/handlers/algorithm_handler.hpp"
#include "server/to_result.hpp"

#include "common/graph_transform.hpp"

#include "ev/fpc_dijkstra.hpp"
#include "ev/graph_transform.hpp"

#include <mutex>
#include <tuple>

namespace charge::server::handlers {

class FPCDijkstra : public AlgorithmHandler {
    inline static constexpr double CHARGING_PENALTY = 60.0;

  public:
    FPCDijkstra(const ev::TradeoffGraph &tradeoff_graph, const double capacity,
                const ev::ChargingFunctionContainer &charging_functions,
                const std::vector<common::Coordinate> &coordinates,
                const std::vector<std::int32_t> &heights)
        : coordinates{coordinates},
          reverse_min_duration_graph(common::invert(ev::tradeoff_to_min_duration(tradeoff_graph))),
          min_consumption_graph(ev::tradeoff_to_min_consumption(tradeoff_graph)),
          shifted_consumption_potentials(ev::shift_negative_weights(min_consumption_graph, heights)),
          reverse_min_consumption_graph(common::invert(min_consumption_graph)),
          omega_graph(ev::tradeoff_to_omega_graph(tradeoff_graph, charging_functions.get_min_chargin_rate(CHARGING_PENALTY))),
          shifted_omega_potentials(ev::shift_negative_weights(omega_graph, heights)),
          reverse_omega_graph(common::invert(omega_graph)),
          context(0.1, 1.0, capacity, CHARGING_PENALTY,
                  charging_functions.get_min_chargin_rate(CHARGING_PENALTY), tradeoff_graph,
                  charging_functions, reverse_min_duration_graph, reverse_min_consumption_graph,
                  shifted_consumption_potentials,
                  reverse_omega_graph, shifted_omega_potentials){}
    // context(0.1, 1.0, capacity, tradeoff_graph, charging_functions, reverse_min_duration_graph)
    // {}

    std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target,
                                   bool search_space) const override final;

    const std::vector<common::Coordinate> &coordinates;
    ev::DurationGraph reverse_min_duration_graph;
    ev::ConsumptionGraph min_consumption_graph;
    std::vector<std::int32_t> shifted_consumption_potentials;
    ev::ConsumptionGraph reverse_min_consumption_graph;
    ev::OmegaGraph omega_graph;
    std::vector<std::int32_t> shifted_omega_potentials;
    ev::OmegaGraph reverse_omega_graph;

    // protected by this mutex
    mutable std::mutex query_mutex;
    mutable ev::FPCAStarLazyOmegaContext context;
    // mutable ev::FPCAStarFastestContext context;
};

std::vector<RouteResult> FPCDijkstra::route(std::uint32_t start, std::uint32_t target,
                                            bool search_space) const {
    std::lock_guard<std::mutex> lock(query_mutex);

    auto solutions = context(start, target);

    std::vector<RouteResult> results;
    for (const auto &solution : solutions) {
        results.push_back(to_result(start, target, solution, context.labels));
        if (search_space) {
            results.back().search_space =
                common::get_search_space(context.labels, context.chargers, coordinates);
        }
    }
    return results;
}

class FPCProfileDijkstra : public AlgorithmHandler {
    inline static constexpr double CHARGING_PENALTY = 60.0;

  public:
    FPCProfileDijkstra(const ev::TradeoffGraph &tradeoff_graph, const double capacity,
                       const ev::ChargingFunctionContainer &charging_functions,
                       const std::vector<common::Coordinate> &coordinates,
                       const std::vector<std::int32_t> &heights)
        : coordinates{coordinates},
          reverse_min_duration_graph(common::invert(ev::tradeoff_to_min_duration(tradeoff_graph))),
          min_consumption_graph(ev::tradeoff_to_min_consumption(tradeoff_graph)),
          shifted_consumption_potentials(ev::shift_negative_weights(min_consumption_graph, heights)),
          reverse_min_consumption_graph(common::invert(min_consumption_graph)),
          omega_graph(ev::tradeoff_to_omega_graph(tradeoff_graph, charging_functions.get_min_chargin_rate(CHARGING_PENALTY))),
          shifted_omega_potentials(ev::shift_negative_weights(omega_graph, heights)),
          reverse_omega_graph(common::invert(omega_graph)),
          context(0.1, 1.0, capacity, CHARGING_PENALTY,
                  charging_functions.get_min_chargin_rate(CHARGING_PENALTY), tradeoff_graph,
                  charging_functions, reverse_min_duration_graph, reverse_min_consumption_graph,
                  shifted_consumption_potentials, reverse_omega_graph, shifted_omega_potentials) {}

    std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target,
                                   bool search_space) const override final;

    const std::vector<common::Coordinate> &coordinates;
    ev::DurationGraph reverse_min_duration_graph;
    ev::ConsumptionGraph min_consumption_graph;
    std::vector<std::int32_t> shifted_consumption_potentials;
    ev::ConsumptionGraph reverse_min_consumption_graph;
    ev::OmegaGraph omega_graph;
    std::vector<std::int32_t> shifted_omega_potentials;
    ev::OmegaGraph reverse_omega_graph;

    // protected by this mutex
    mutable std::mutex query_mutex;
    mutable ev::FPCProfileAStarLazyOmegaContext context;
};

std::vector<RouteResult> FPCProfileDijkstra::route(std::uint32_t start, std::uint32_t target,
                                                   bool search_space) const {
    std::lock_guard<std::mutex> lock(query_mutex);

    auto solutions = context(start, target);

    std::vector<RouteResult> results;
    for (const auto &solution : solutions) {
        results.push_back(to_result(start, target, solution, context.labels));
        if (search_space) {
            results.back().search_space =
                common::get_search_space(context.labels, context.chargers, coordinates);
        }
    }
    return results;
}
} // namespace charge::server::handlers

#endif
