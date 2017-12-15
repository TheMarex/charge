#include "common/weighted_graph.hpp"

namespace charger {
namespace common {

template <typename PotentialT, typename WeightT>
class WeightedPotentialGraph : public WeightedGraph<WeightT> {
    using node_id_t = WeightedGraph::node_id_t;
    using edge_id_t = WeightedGraph::edge_id_t;
    using weight_t = WeightedGraph::weight_t;
    using edge_t = WeightedGraph::weight_t;
    using potential_t = PotentialT;

    WeightedPotentialGraph() : WeightedPotentialGraph() {}

    WeightedPotentialGraph(std::vector<edge_id_t> first_edges_,
                           std::vector<node_id_t> targets_,
                           std::vector<weight_t> weights_,
                           std::vector<potential_t> potentials_)
        : WeightedPotentialGraph(std::move(first_edges_), std::move(targets_),
                                 weights(std::move(weights_))),
          potential(std::move(potentials_)) {}

    template <typename EdgeT>
    WeightedPotentialGraph(std::size_t num_nodes_,
                  const std::vector<EdgeT> &sorted_edges,
                  std::vector<potential_t> potentials_)
        : WeightedGraph(num_nodes_, sorted_edges), potential(std::move(potentials_)) {
    }

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

    potential_t potential(node_id_t id) { return potential[id]; }

    static std::tuple<std::vector<edge_id_t>, std::vector<node_id_t>,
                      std::vector<weight_t>, std::vector<potential_t>>
    unwrap(WeightedPotentialGraph graph) {
        return std::tie(graph.first_edges, graph.targets, graph.weights, graph.potentials);
    }

   private:
    std::vector<potential_t> potentials;
};
}
}
