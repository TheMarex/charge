#ifndef CHARGE_EV_FPC_DIJKSTRA_HPP
#define CHARGE_EV_FPC_DIJKSTRA_HPP

#include "ev/fp_dijkstra.hpp"

namespace charge::ev {
template <typename LabelEntryT>
class TradeoffChargingDijkstraPolicy
    : public TradeoffDijkstraPolicy<LabelEntryT>,
      public common::FPDijkstraWeightedNodePolicy<TradeoffGraph::Base, LabelEntryT,
                                                  ChargingFunctionContainer> {
  public:
    using TradeoffBase = TradeoffDijkstraPolicy<LabelEntryT>;
    using WeightedNodeBase = common::FPDijkstraWeightedNodePolicy<TradeoffGraph::Base, LabelEntryT,
                                                                  ChargingFunctionContainer>;

    TradeoffChargingDijkstraPolicy(const double capacity,
                                   const ChargingFunctionContainer &node_weights)
        : TradeoffChargingDijkstraPolicy(0.1, 1.0, capacity, 60., node_weights) {}

    TradeoffChargingDijkstraPolicy(const double x_eps, const double y_eps, const double capacity,
                                   const double charging_penalty,
                                   const ChargingFunctionContainer &node_weights)
        : TradeoffBase{x_eps, y_eps, capacity}, WeightedNodeBase{node_weights, charging_penalty} {}

    using label_t = LabelEntryT;
    using cost_t = typename label_t::cost_t;
    using weight_t = typename TradeoffGraph::weight_t;
    using node_id_t = typename TradeoffGraph::node_id_t;
    using queue_t = common::MinIDQueue;
    using key_t = std::uint32_t;

    using TradeoffBase::clip_dominated;
    using TradeoffBase::constrain;
    using TradeoffBase::dominates;
    using TradeoffBase::link;
    using TradeoffBase::terminate;

    using WeightedNodeBase::WeightedNodeBase;
    using WeightedNodeBase::link_node;
    using WeightedNodeBase::weighted;
};

template <typename LabelEntryT>
class TradeoffChargingProfileDijkstraPolicy : public TradeoffChargingDijkstraPolicy<LabelEntryT> {
  public:
    using TradeoffBase = TradeoffChargingDijkstraPolicy<LabelEntryT>;
    using TradeoffBase::TradeoffBase;
    using label_t = LabelEntryT;
    using cost_t = typename label_t::cost_t;
    using weight_t = typename TradeoffGraph::weight_t;
    using node_id_t = typename TradeoffGraph::node_id_t;
    using queue_t = common::MinIDQueue;
    using key_t = std::uint32_t;

    // don't terminate, wait until heap is empty to get the full profile
    template <typename NodeLabelsT>
    static bool terminate(const common::MinIDQueue &, const NodeLabelsT &,
                          const typename TradeoffBase::node_id_t) {
        return false;
    }
};

using TradeoffChargingDijkstraPolicyWithParents = TradeoffChargingDijkstraPolicy<
    common::LabelEntryWithParent<TradeoffGraph::weight_t, TradeoffGraph::node_id_t>>;

using TradeoffChargingProfileDijkstraPolicyWithParents = TradeoffChargingProfileDijkstraPolicy<
    common::LabelEntryWithParent<TradeoffGraph::weight_t, TradeoffGraph::node_id_t>>;

template <typename LabelEntryT>
auto fpc_dijkstra(const ev::TradeoffGraph::node_id_t start,
                  const ev::TradeoffGraph::node_id_t target, const ev::TradeoffGraph &graph,
                  const ChargingFunctionContainer &chargers, common::MinIDQueue &queue,
                  common::NodeLabels<TradeoffChargingDijkstraPolicy<LabelEntryT>> &labels,
                  const double capacity = std::numeric_limits<double>::infinity(),
                  const double x_eps = 0.1, const double y_eps = 1.0,
                  const double charging_penalty = 60.) {
    using Policy = TradeoffChargingDijkstraPolicy<LabelEntryT>;
    using NodePotentials = common::ZeroNodePotentials<ev::TradeoffGraph>;
    const auto &query_graph =
        static_cast<const common::WeightedGraph<ev::LimitedTradeoffFunction> &>(graph);
    return common::fp_dijkstra(start, target, ev::make_constant(0, 0), query_graph, queue, labels,
                               NodePotentials{},
                               Policy{x_eps, y_eps, capacity, charging_penalty, chargers});
}

template <typename LabelEntryT, typename NodePotentialsT>
auto fpc_astar(const ev::TradeoffGraph::node_id_t start, const ev::TradeoffGraph::node_id_t target,
               const ev::TradeoffGraph &graph, const ChargingFunctionContainer &chargers,
               const NodePotentialsT &potentials, common::MinIDQueue &queue,
               common::NodeLabels<TradeoffChargingDijkstraPolicy<LabelEntryT>> &labels,
               const double capacity = std::numeric_limits<double>::infinity(),
               const double x_eps = 0.1, const double y_eps = 1.0,
               const double charging_penalty = 60.) {
    using Policy = TradeoffChargingDijkstraPolicy<LabelEntryT>;
    const auto &query_graph =
        static_cast<const common::WeightedGraph<ev::LimitedTradeoffFunction> &>(graph);
    return common::fp_dijkstra(start, target, ev::make_constant(0, 0), query_graph, queue, labels,
                               potentials,
                               Policy{x_eps, y_eps, capacity, charging_penalty, chargers});
}

template <typename LabelEntryT, typename NodePotentialsT>
auto fpc_profile_astar(
    const ev::TradeoffGraph::node_id_t start, const ev::TradeoffGraph::node_id_t target,
    const ev::TradeoffGraph &graph, const ChargingFunctionContainer &chargers,
    const NodePotentialsT &potentials, common::MinIDQueue &queue,
    common::NodeLabels<TradeoffChargingProfileDijkstraPolicy<LabelEntryT>> &labels,
    const double capacity = std::numeric_limits<double>::infinity(), const double x_eps = 0.1,
    const double y_eps = 1.0, const double charging_penalty = 60.) {
    using Policy = TradeoffChargingProfileDijkstraPolicy<LabelEntryT>;
    const auto &query_graph =
        static_cast<const common::WeightedGraph<ev::LimitedTradeoffFunction> &>(graph);
    return common::fp_dijkstra(start, target, ev::make_constant(0, 0), query_graph, queue, labels,
                               potentials,
                               Policy{x_eps, y_eps, capacity, charging_penalty, chargers});
}

// Function propagation using chargers with Dijkstra
struct FPCDijkstraContext {
    FPCDijkstraContext(const double x_eps, const double y_eps, const double capacity,
                       const double charging_penalty, const TradeoffGraph &graph,
                       const ChargingFunctionContainer &chargers)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()), labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCDijkstraContext(FPCDijkstraContext &&) = default;
    FPCDijkstraContext(const FPCDijkstraContext &) = default;
    FPCDijkstraContext &operator=(FPCDijkstraContext &&) = default;
    FPCDijkstraContext &operator=(const FPCDijkstraContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        return fpc_dijkstra(start, target, graph, chargers, queue, labels, capacity, x_eps, y_eps,
                            charging_penalty);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer chargers;
    common::MinIDQueue queue;
    common::NodeLabels<TradeoffChargingDijkstraPolicyWithParents> labels;
};

// Function propagation using chargers with A*
struct FPCAStarOmegaContext {
    FPCAStarOmegaContext(const double x_eps, const double y_eps, const double capacity,
                         const double charging_penalty, const double min_charging_rate,
                         const TradeoffGraph &graph, const ChargingFunctionContainer &chargers,
                         const DurationGraph &reverse_min_duration_graph,
                         const ConsumptionGraph &reverse_consumption_graph,
                         const OmegaGraph &reverse_omega_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()),
          potentials(capacity, min_charging_rate, reverse_min_duration_graph,
                     reverse_consumption_graph, reverse_omega_graph),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCAStarOmegaContext(FPCAStarOmegaContext &&) = default;
    FPCAStarOmegaContext(const FPCAStarOmegaContext &) = default;
    FPCAStarOmegaContext &operator=(FPCAStarOmegaContext &&) = default;
    FPCAStarOmegaContext &operator=(const FPCAStarOmegaContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return fpc_astar(start, target, graph, chargers, potentials, queue, labels, capacity, x_eps,
                         y_eps, charging_penalty);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    ev::OmegaNodePotentials potentials;
    common::NodeLabels<TradeoffChargingDijkstraPolicyWithParents> labels;
};

// Function propagation using chargers with A*
struct FPCAStarLazyOmegaContext {
    FPCAStarLazyOmegaContext(const double x_eps, const double y_eps, const double capacity,
                    const double charging_penalty, const double min_charging_rate,
                    const TradeoffGraph &graph, const ChargingFunctionContainer &chargers,
                    const DurationGraph &reverse_min_duration_graph,
                    const ConsumptionGraph &reverse_consumption_graph,
                    const std::vector<std::int32_t>& shifted_consumption_potentials,
                    const OmegaGraph &reverse_omega_graph,
                    const std::vector<std::int32_t>& shifted_omega_potentials)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()),
          potentials(capacity, min_charging_rate, reverse_min_duration_graph,
                     reverse_consumption_graph, 
                     shifted_consumption_potentials,
                     reverse_omega_graph,
                     shifted_omega_potentials),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCAStarLazyOmegaContext(FPCAStarLazyOmegaContext &&) = default;
    FPCAStarLazyOmegaContext(const FPCAStarLazyOmegaContext &) = default;
    FPCAStarLazyOmegaContext &operator=(FPCAStarLazyOmegaContext &&) = default;
    FPCAStarLazyOmegaContext &operator=(const FPCAStarLazyOmegaContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(start, target);
        return fpc_astar(start, target, graph, chargers, potentials, queue, labels, capacity, x_eps,
                         y_eps, charging_penalty);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    ev::LazyOmegaNodePotentials potentials;
    common::NodeLabels<TradeoffChargingDijkstraPolicyWithParents> labels;
};

// Function propagation using chargers with A*
struct FPCProfileAStarLazyOmegaContext {
    FPCProfileAStarLazyOmegaContext(const double x_eps, const double y_eps, const double capacity,
                    const double charging_penalty, const double min_charging_rate,
                    const TradeoffGraph &graph, const ChargingFunctionContainer &chargers,
                    const DurationGraph &reverse_min_duration_graph,
                    const ConsumptionGraph &reverse_consumption_graph,
                    const std::vector<std::int32_t>& shifted_consumption_potentials,
                    const OmegaGraph &reverse_omega_graph,
                    const std::vector<std::int32_t>& shifted_omega_potentials)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()),
          potentials(capacity, min_charging_rate, reverse_min_duration_graph,
                     reverse_consumption_graph, 
                     shifted_consumption_potentials,
                     reverse_omega_graph,
                     shifted_omega_potentials),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCProfileAStarLazyOmegaContext(FPCProfileAStarLazyOmegaContext &&) = default;
    FPCProfileAStarLazyOmegaContext(const FPCProfileAStarLazyOmegaContext &) = default;
    FPCProfileAStarLazyOmegaContext &operator=(FPCProfileAStarLazyOmegaContext &&) = default;
    FPCProfileAStarLazyOmegaContext &operator=(const FPCProfileAStarLazyOmegaContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(start, target);
        return fpc_profile_astar(start, target, graph, chargers, potentials, queue, labels,
                                 capacity, x_eps, y_eps, charging_penalty);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    ev::LazyOmegaNodePotentials potentials;
    common::NodeLabels<TradeoffChargingProfileDijkstraPolicyWithParents> labels;
};

// Function propagation with A*
struct FPCAStarFastestContext {
    FPCAStarFastestContext(const double x_eps, const double y_eps, const double capacity,
                           const double charging_penalty, const TradeoffGraph &graph,
                           const ChargingFunctionContainer &chargers,
                           const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()),
          potentials(reverse_min_duration_graph), labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCAStarFastestContext(FPCAStarFastestContext &&) = default;
    FPCAStarFastestContext(const FPCAStarFastestContext &) = default;
    FPCAStarFastestContext &operator=(FPCAStarFastestContext &&) = default;
    FPCAStarFastestContext &operator=(const FPCAStarFastestContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return fpc_astar(start, target, graph, chargers, potentials, queue, labels, capacity, x_eps,
                         y_eps, charging_penalty);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    common::LandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<TradeoffChargingDijkstraPolicyWithParents> labels;
};

// Function propagation with A*
struct FPCAStarLazyFastestContext {
    FPCAStarLazyFastestContext(const double x_eps, const double y_eps, const double capacity,
                               const double charging_penalty, const TradeoffGraph &graph,
                               const ChargingFunctionContainer &chargers,
                               const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), charging_penalty(charging_penalty),
          graph(graph), chargers(chargers), queue(graph.num_nodes()),
          potentials(reverse_min_duration_graph), labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPCAStarLazyFastestContext(FPCAStarLazyFastestContext &&) = default;
    FPCAStarLazyFastestContext(const FPCAStarLazyFastestContext &) = default;
    FPCAStarLazyFastestContext &operator=(FPCAStarLazyFastestContext &&) = default;
    FPCAStarLazyFastestContext &operator=(const FPCAStarLazyFastestContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(start, target);
        return fpc_astar(start, target, graph, chargers, potentials, queue, labels, capacity, x_eps,
                         y_eps);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const double charging_penalty;
    const TradeoffGraph &graph;
    const ChargingFunctionContainer &chargers;
    common::MinIDQueue queue;
    common::LazyLandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<TradeoffChargingDijkstraPolicyWithParents> labels;
};
} // namespace charge::ev

#endif
