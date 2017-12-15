#ifndef CHARGE_SERVER_HANDLERS_MCC_DIJKSTRA_HPP
#define CHARGE_SERVER_HANDLERS_MCC_DIJKSTRA_HPP

#include "server/handlers/algorithm_handler.hpp"
#include "server/to_result.hpp"

#include "common/graph_transform.hpp"

#include "ev/graph.hpp"
#include "ev/graph_transform.hpp"
#include "ev/mcc_dijkstra.hpp"

#include <mutex>
#include <tuple>

namespace charge::server::handlers {

class MCCDijkstra : public AlgorithmHandler {

    inline static constexpr double SAMPLE_RESOLUTION = 10.0;
    inline static constexpr double CHARGING_PENALTY = 60.0;

  public:
    MCCDijkstra(const ev::TradeoffGraph &tradeoff_graph, const double capacity,
                const ev::ChargingFunctionContainer &charging_functions,
                const std::vector<common::Coordinate> &coordinates)
        : coordinates{coordinates}, graph(ev::tradeoff_to_sampled_consumption(tradeoff_graph, SAMPLE_RESOLUTION)),
          reverse_min_duration_graph(common::invert(ev::tradeoff_to_min_duration(tradeoff_graph))),
          context(0.1, 1.0, SAMPLE_RESOLUTION, capacity, CHARGING_PENALTY, graph, charging_functions,
                  reverse_min_duration_graph) {}

    std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target, bool search_space) const override final;

    const std::vector<common::Coordinate> &coordinates;
    const ev::DurationConsumptionGraph graph;
    const ev::DurationGraph reverse_min_duration_graph;

    // protected by this mutex
    mutable std::mutex query_mutex;
    mutable ev::MCCAStarFastestContext context;
};

std::vector<RouteResult> MCCDijkstra::route(std::uint32_t start, std::uint32_t target, bool search_space) const {
    std::lock_guard<std::mutex> lock(query_mutex);

    auto solutions = context(start, target);

    std::vector<RouteResult> results;
    for (const auto &solution : solutions) {
        results.push_back(to_result(start, target, solution, context.labels));
        if (search_space)
        {
            results.back().search_space = common::get_search_space(context.labels, context.chargers, coordinates);
        }
    }
    return results;
}
} // namespace charge::server::handlers

#endif
