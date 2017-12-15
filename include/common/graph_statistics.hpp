#ifndef CHAGRE_COMMON_GRAPH_STATISTICS_HPP
#define CHAGRE_COMMON_GRAPH_STATISTICS_HPP

#include "ev/graph.hpp"

#include <sstream>

namespace charge::common {
 std::string get_statistics(const ev::TradeoffGraph &graph) {
    std::stringstream ss;
    std::size_t tradeoff_edges = 0;
    std::size_t negative_edge = 0;
    for (const auto node : graph.nodes()) {
        for (const auto edge : graph.charge::common::AdjGraph::edges(node)) {
            const auto &w = graph.weight(edge);
            if (w->is_hyperbolic()) {
                tradeoff_edges++;
            }
            if (w(w.max_x) < 0)
                negative_edge++;
        }
    }
    ss << "Graph has " << graph.num_nodes() << " nodes and " << graph.num_edges() << " edges ("
       << tradeoff_edges << " hyperbolic " << negative_edge <<  " negative)." << std::endl;

    return ss.str();
}

 std::string get_statistics(const ev::DurationConsumptionGraph &graph) {
    std::stringstream ss;
    ss << "Graph has " << graph.num_nodes() << " nodes and " << graph.num_edges() << " edges. " << std::endl;

    return ss.str();
}
}

#endif
