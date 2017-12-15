#ifndef CHARGE_SERVER_HANDLERS_DIJKSTRA_HPP
#define CHARGE_SERVER_HANDLERS_DIJKSTRA_HPP

#include "server/handlers/algorithm_handler.hpp"
#include "server/to_result.hpp"

#include "common/files.hpp"
#include "common/graph_transform.hpp"
#include "common/path.hpp"

#include "ev/dijkstra.hpp"
#include "ev/graph_transform.hpp"

#include <mutex>

namespace charge::server::handlers {

class Dijkstra : public AlgorithmHandler {
  public:
    Dijkstra(const ev::TradeoffGraph &tradeoff_graph)
        : graph(ev::tradeoff_to_min_duration(tradeoff_graph)),
          context(tradeoff_graph, graph) {}

    std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target, bool search_space) const override final;

    const ev::DurationGraph graph;

    // protected by this mutex
    mutable std::mutex query_mutex;
    mutable ev::MinDurationDijkstraContetx context;
};

std::vector<RouteResult> Dijkstra::route(std::uint32_t start, std::uint32_t target, bool) const {
    std::lock_guard<std::mutex> lock(query_mutex);

    auto cost = context(start, target);

    if (cost == common::INF_WEIGHT) {
        return {};
    }

    return {to_result(start, target, context.tradeoff_graph, context.costs, context.parents)};
}
}

#endif
