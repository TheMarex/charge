#ifndef CHARGE_COMMON_WEIGHTED_GRAPH_HPP
#define CHARGE_COMMON_WEIGHTED_GRAPH_HPP

#include "common/adj_graph.hpp"

namespace charge {
namespace common {

template <typename WeightT>
class WeightedGraph : public AdjGraph {
   public:
    using node_id_t = AdjGraph::node_id_t;
    using edge_id_t = AdjGraph::edge_id_t;
    using weight_t = WeightT;
    using edge_t = Edge<node_id_t, weight_t>;

    WeightedGraph() : AdjGraph() {}

    WeightedGraph(std::vector<edge_id_t> first_edges_,
                  std::vector<node_id_t> targets_,
                  std::vector<weight_t> weights_)
        : AdjGraph(std::move(first_edges_), std::move(targets_)),
          weights(std::move(weights_)) {}

    template <typename EdgeT>
    WeightedGraph(std::size_t num_nodes_,
                  const std::vector<EdgeT> &sorted_edges)
        : AdjGraph(num_nodes_, sorted_edges) {
        assert(std::is_sorted(
            sorted_edges.begin(), sorted_edges.end(), [](const auto &lhs, const auto &rhs) {
                return std::tie(lhs.start, lhs.target) < std::tie(rhs.start, rhs.target);
            }));

        for (const auto &edge : sorted_edges) {
            weights.push_back(edge.weight);
        }

        assert(weights.size() == sorted_edges.size());
    }

    using AdjGraph::edges;

    std::vector<edge_t> edges() const {
        std::vector<edge_t> edges;
        edges.reserve(num_edges());

        for (node_id_t node = 0; node < num_nodes(); ++node) {
            for (auto edge = begin(node); edge < end(node); ++edge) {
                edges.emplace_back(node, target(edge), weight(edge));
            }
        }

        return edges;
    }

    const weight_t& weight(edge_id_t id) const { return weights[id]; }
    weight_t& weight(edge_id_t id) { return weights[id]; }

    static std::tuple<std::vector<edge_id_t>, std::vector<node_id_t>,
                      std::vector<weight_t>>
    unwrap(WeightedGraph graph) {
        return std::tie(graph.first_edges, graph.targets, graph.weights);
    }

   protected:
    std::vector<weight_t> weights;
};
}
}

#endif
