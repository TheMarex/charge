#ifndef CHARGE_EV_MCC_DIJKSTRA_HPP
#define CHARGE_EV_MCC_DIJKSTRA_HPP

#include "ev/charging_function_container.hpp"
#include "ev/mc_dijkstra.hpp"

namespace charge::ev {

template <typename LabelEntryT>
class DurationConsumptionChargingDijkstraPolicy
    : public DurationConsumptionDijkstraPolicy<LabelEntryT>,
      public common::MCDijkstraWeightedNodePolicy<DurationConsumptionGraph::Base, LabelEntryT,
                                                  ChargingFunctionContainer> {
  public:
    using DurationConsumptionBase = DurationConsumptionDijkstraPolicy<LabelEntryT>;
    using WeightedNodeBase =
        common::MCDijkstraWeightedNodePolicy<DurationConsumptionGraph::Base, LabelEntryT,
                                             ChargingFunctionContainer>;

    DurationConsumptionChargingDijkstraPolicy(const std::int32_t x_eps, const std::int32_t y_eps,
                                              const std::int32_t capacity,
                                              const std::int32_t charging_penalty,
                                              const ChargingFunctionContainer &node_weights,
                                              const double sample_resolution = 10.0)
        : DurationConsumptionBase{x_eps, y_eps, capacity}, WeightedNodeBase{node_weights,
                                                                            sample_resolution,
                                                                            charging_penalty} {}

    DurationConsumptionChargingDijkstraPolicy(const std::int32_t capacity,
                                              const ChargingFunctionContainer &node_weights)
        : DurationConsumptionChargingDijkstraPolicy(common::to_fixed(0.1), common::to_fixed(1.0),
                                                    common::to_fixed(60), capacity, node_weights) {}

    using label_t = LabelEntryT;
    using cost_t = typename label_t::cost_t;
    using weight_t = typename DurationConsumptionGraph::weight_t;
    using node_id_t = typename DurationConsumptionGraph::node_id_t;
    using queue_t = common::MinIDQueue;
    using key_t = std::uint32_t;
    using graph_t = DurationConsumptionGraph;

    using DurationConsumptionBase::clip_dominated;
    using DurationConsumptionBase::constrain;
    using DurationConsumptionBase::dominates;
    using DurationConsumptionBase::enable_stalling;
    using DurationConsumptionBase::key;
    using DurationConsumptionBase::link;
    using DurationConsumptionBase::terminate;

    using WeightedNodeBase::WeightedNodeBase;
    using WeightedNodeBase::link_node;
    using WeightedNodeBase::weighted;
};

using DurationConsumptionChargingDijkstraPolicyWithParents =
    DurationConsumptionChargingDijkstraPolicy<DurationConsumptionLabelEntryWithParent>;

// Runs a Multi-Criteria dijkstra with Pareto-Dominanz on a bi-criterial graph
template <typename LabelEntryT>
auto mcc_dijkstra(
    const DurationConsumptionGraph::node_id_t start,
    const DurationConsumptionGraph::node_id_t target, const DurationConsumptionGraph &graph,
    const ChargingFunctionContainer &chargers, common::MinIDQueue &queue,
    common::NodeLabels<DurationConsumptionChargingDijkstraPolicy<LabelEntryT>> &labels,
    const std::int32_t capacity = common::INF_WEIGHT,
    const std::int32_t x_eps = common::to_fixed(0.1),
    const std::int32_t y_eps = common::to_fixed(1.0), const double sample_resolution = 10.0,
    const std::int32_t charging_penalty = common::to_fixed(60)) {
    using Policy = DurationConsumptionChargingDijkstraPolicy<LabelEntryT>;
    return common::mc_dijkstra(start, target, graph, queue, labels,
                               common::ZeroNodePotentials<DurationConsumptionGraph>{},
                               Policy{x_eps, y_eps, capacity, charging_penalty, chargers, sample_resolution});
}

template <typename LabelEntryT, typename NodePotentialT>
auto mcc_astar(const DurationConsumptionGraph::node_id_t start,
               const DurationConsumptionGraph::node_id_t target,
               const DurationConsumptionGraph &graph, const ChargingFunctionContainer &chargers,
               common::MinIDQueue &queue,
               common::NodeLabels<DurationConsumptionChargingDijkstraPolicy<LabelEntryT>> &labels,
               const NodePotentialT &potentials, const std::int32_t capacity = common::INF_WEIGHT,
               const std::int32_t x_eps = common::to_fixed(0.1),
               const std::int32_t y_eps = common::to_fixed(1.0),
               const double sample_resolution = 10.0,
               const std::int32_t charging_penalty = common::to_fixed(60)) {
    using Policy = DurationConsumptionChargingDijkstraPolicy<LabelEntryT>;
    return common::mc_dijkstra(start, target, graph, queue, labels, potentials,
                               Policy{x_eps, y_eps, capacity, charging_penalty, chargers, sample_resolution});
}

// Multi-Criteria with Dijkstra
struct MCCDijkstraContext {
    MCCDijkstraContext(const double x_eps, const double y_eps, const double sample_resolution,
                       const double capacity, const double charging_penalty,
                       const DurationConsumptionGraph &graph,
                       const ChargingFunctionContainer &chargers)
        : x_eps(x_eps), y_eps(y_eps), sample_resolution(sample_resolution), capacity(capacity),
          charging_penalty(charging_penalty), graph(graph), chargers(chargers),
          queue(graph.num_nodes()), labels(graph.num_nodes()) {}

    // Make copyable and movable
    MCCDijkstraContext(MCCDijkstraContext &&) = default;
    MCCDijkstraContext(const MCCDijkstraContext &) = default;
    MCCDijkstraContext &operator=(MCCDijkstraContext &&) = default;
    MCCDijkstraContext &operator=(const MCCDijkstraContext &) = default;

    auto operator()(const DurationConsumptionGraph::node_id_t start,
                    const DurationConsumptionGraph::node_id_t target) {
        return mcc_dijkstra(start, target, graph, chargers, queue, labels,
                            common::to_fixed(capacity), common::to_fixed(x_eps),
                            common::to_fixed(y_eps), common::to_fixed(charging_penalty));
    }

    const double x_eps;
    const double y_eps;
    const double sample_resolution;
    const double capacity;
    const double charging_penalty;
    const DurationConsumptionGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    common::NodeLabels<DurationConsumptionChargingDijkstraPolicyWithParents> labels;
};

// Multi-Criteria with A*
struct MCCAStarFastestContext {
    MCCAStarFastestContext(const double x_eps, const double y_eps, const double sample_resolution,
                    const double capacity, const double charging_penalty,
                    const DurationConsumptionGraph &graph,
                    const ChargingFunctionContainer &chargers,
                    const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), sample_resolution(sample_resolution), capacity(capacity),
          charging_penalty(charging_penalty), graph(graph), chargers(chargers),
          queue(graph.num_nodes()), potentials(reverse_min_duration_graph),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    MCCAStarFastestContext(MCCAStarFastestContext &&) = default;
    MCCAStarFastestContext(const MCCAStarFastestContext &) = default;
    MCCAStarFastestContext &operator=(MCCAStarFastestContext &&) = default;
    MCCAStarFastestContext &operator=(const MCCAStarFastestContext &) = default;

    auto operator()(const DurationConsumptionGraph::node_id_t start,
                    const DurationConsumptionGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return mcc_astar(start, target, graph, chargers, queue, labels, potentials,
                         common::to_fixed(capacity), common::to_fixed(x_eps),
                         common::to_fixed(y_eps), common::to_fixed(charging_penalty));
    }

    const double x_eps;
    const double y_eps;
    const double sample_resolution;
    const double capacity;
    const double charging_penalty;
    const DurationConsumptionGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    common::LandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<DurationConsumptionChargingDijkstraPolicyWithParents> labels;
};

// Multi-Criteria with A*
struct MCCAStarLazyFastestContext {
    MCCAStarLazyFastestContext(const double x_eps, const double y_eps, const double sample_resolution,
                    const double capacity, const double charging_penalty,
                    const DurationConsumptionGraph &graph,
                    const ChargingFunctionContainer &chargers,
                    const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), sample_resolution(sample_resolution), capacity(capacity),
          charging_penalty(charging_penalty), graph(graph), chargers(chargers),
          queue(graph.num_nodes()), potentials(reverse_min_duration_graph),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    MCCAStarLazyFastestContext(MCCAStarLazyFastestContext &&) = default;
    MCCAStarLazyFastestContext(const MCCAStarLazyFastestContext &) = default;
    MCCAStarLazyFastestContext &operator=(MCCAStarLazyFastestContext &&) = default;
    MCCAStarLazyFastestContext &operator=(const MCCAStarLazyFastestContext &) = default;

    auto operator()(const DurationConsumptionGraph::node_id_t start,
                    const DurationConsumptionGraph::node_id_t target) {
        potentials.recompute(start, target);
        return mcc_astar(start, target, graph, chargers, queue, labels, potentials,
                         common::to_fixed(capacity), common::to_fixed(x_eps),
                         common::to_fixed(y_eps), common::to_fixed(charging_penalty));
    }

    const double x_eps;
    const double y_eps;
    const double sample_resolution;
    const double capacity;
    const double charging_penalty;
    const DurationConsumptionGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    common::LazyLandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<DurationConsumptionChargingDijkstraPolicyWithParents> labels;
};
} // namespace charge::ev

#endif
