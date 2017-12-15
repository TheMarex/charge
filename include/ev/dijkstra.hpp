#ifndef CHARGE_EV_DIJKSTRA_HPP
#define CHARGE_EV_DIJKSTRA_HPP

#include "common/dijkstra.hpp"

#include "ev/graph.hpp"

namespace charge::ev {

// Single-Criteria with Dijkstra
template <typename GraphT> struct DijkstraContext {
    DijkstraContext(const TradeoffGraph &tradeoff_graph, const GraphT &graph)
        : tradeoff_graph(tradeoff_graph), graph(graph), queue(graph.num_nodes()),
          costs(graph.num_nodes(), common::INF_WEIGHT),
          parents(graph.num_nodes(), common::INVALID_ID) {}

    // Make copyable and movable
    DijkstraContext(DijkstraContext &&) = default;
    DijkstraContext(const DijkstraContext &) = default;
    DijkstraContext &operator=(DijkstraContext &&) = default;
    DijkstraContext &operator=(const DijkstraContext &) = default;

    auto operator()(const typename GraphT::node_id_t start,
                    const typename GraphT::node_id_t target) {
        return common::dijkstra(start, target, graph, queue, costs, parents);
    }

    const TradeoffGraph &tradeoff_graph;
    const GraphT &graph;
    common::MinIDQueue queue;
    common::CostVector<GraphT> costs;
    common::ParentVector<GraphT> parents;
};

using MinDurationDijkstraContetx = DijkstraContext<DurationGraph>;
using MinConsumptionDijkstraContetx = DijkstraContext<ConsumptionGraph>;
}

#endif
