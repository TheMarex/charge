#ifndef CHARGE_SERVER_HANDLERS_FP_DIJKSTRA_HPP
#define CHARGE_SERVER_HANDLERS_FP_DIJKSTRA_HPP

#include "server/handlers/algorithm_handler.hpp"
#include "server/to_result.hpp"

#include "common/graph_transform.hpp"

#include "ev/fp_dijkstra.hpp"
#include "ev/graph_transform.hpp"

#include <mutex>
#include <tuple>

namespace charge::server::handlers {

class FPDijkstra : public AlgorithmHandler {
  public:
    FPDijkstra(const ev::TradeoffGraph &tradeoff_graph, const double capacity,
               const std::vector<common::Coordinate> &coordinates)
        : coordinates{coordinates},
          reverse_min_duration_graph(common::invert(ev::tradeoff_to_min_duration(tradeoff_graph))),
          context(0.1, 1.0, capacity, tradeoff_graph, reverse_min_duration_graph) {}

    std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target,
                                   bool search_space) const override final;

    const std::vector<common::Coordinate> &coordinates;
    ev::DurationGraph reverse_min_duration_graph;

    // protected by this mutex
    mutable std::mutex query_mutex;
    mutable ev::FPAStarContext context;
};

std::vector<RouteResult> FPDijkstra::route(std::uint32_t start, std::uint32_t target,
                                           bool search_space) const {
    std::lock_guard<std::mutex> lock(query_mutex);

    auto solutions = context(start, target);

    std::vector<RouteResult> results;
    for (const auto &solution : solutions) {
        results.push_back(to_result(start, target, solution, context.labels));
        if (search_space) {
            results.back().search_space = common::get_search_space(context.labels, coordinates);
        }
    }
    return results;
}
} // namespace charge::server::handlers

#endif
