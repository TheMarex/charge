#ifndef CHARGE_EV_FP_DIJKSTRA_HPP
#define CHARGE_EV_FP_DIJKSTRA_HPP

#include "common/constants.hpp"
#include "common/fp_dijkstra.hpp"

#include "ev/charging_function_container.hpp"
#include "ev/graph.hpp"
#include "ev/node_potentials.hpp"

namespace charge::ev {

template <typename LabelEntryT>
class TradeoffDijkstraPolicy
    : public common::FPDijkstraPolicy<ev::TradeoffGraph::Base, LabelEntryT> {
  public:
    using Base = common::FPDijkstraPolicy<ev::TradeoffGraph::Base, LabelEntryT>;

    TradeoffDijkstraPolicy(const double capacity)
        : TradeoffDijkstraPolicy(common::to_fixed(0.1), common::to_fixed(1), capacity) {}

    TradeoffDijkstraPolicy(const double x_epsilon_, const double y_epsilon_, const double capacity)
        : x_epsilon(common::to_fixed(x_epsilon_)), y_epsilon(common::to_fixed(y_epsilon_)),
          capacity(capacity) {}

    // Applies the battery constraints
    bool constrain(typename Base::cost_t &cost) const {
        cost.limit_from_y(0, capacity);
        return cost.functions.empty();
    }

    template <typename NodeLabelsT>
    static bool terminate(const common::MinIDQueue &queue, const NodeLabelsT &labels,
                          const typename Base::node_id_t target) {
        return common::function_propergation_traits::min_key_terminate(queue, labels, target);
    }

    auto dominates(const typename Base::cost_t &lhs, const typename Base::cost_t &rhs) const {
        common::Statistics::get().count(common::StatisticsEvent::DOMINATION);
        return common::function_propergation_traits::dominates(lhs, rhs, x_epsilon, y_epsilon);
    }

    template <typename Range>
    auto dominates(const Range &lhs_range, const typename Base::cost_t &rhs) const {
        common::Statistics::get().count(common::StatisticsEvent::DOMINATION);
        return common::function_propergation_traits::dominates(lhs_range, rhs, x_epsilon,
                                                               y_epsilon);
    }

    auto clip_dominated(const typename Base::cost_t &lhs, typename Base::cost_t &rhs) const {
        common::Statistics::get().count(common::StatisticsEvent::DOMINATION);
        return common::function_propergation_traits::clip_dominated(lhs, rhs, x_epsilon, y_epsilon);
    }

    template <typename Range>
    auto clip_dominated(const Range &lhs_range, typename Base::cost_t &rhs) const {
        common::Statistics::get().count(common::StatisticsEvent::DOMINATION);
        return common::function_propergation_traits::clip_dominated(lhs_range, rhs, x_epsilon,
                                                                    y_epsilon);
    }

    const std::int32_t x_epsilon;
    const std::int32_t y_epsilon;
    const double capacity;
};

using TradeoffLabelEntryWithParent =
    common::LabelEntryWithParent<TradeoffGraph::weight_t, TradeoffGraph::node_id_t>;
using TradeoffDijkstraPolicyWithParents = TradeoffDijkstraPolicy<TradeoffLabelEntryWithParent>;

template <typename LabelEntryT>
auto fp_dijkstra(const ev::TradeoffGraph::node_id_t start,
                 const ev::TradeoffGraph::node_id_t target, const ev::TradeoffGraph &graph,
                 common::MinIDQueue &queue,
                 common::NodeLabels<TradeoffDijkstraPolicy<LabelEntryT>> &labels,
                 const double capacity = std::numeric_limits<double>::infinity(),
                 const double x_eps = 0.1, const double y_eps = 1.0) {
    using Policy = TradeoffDijkstraPolicy<LabelEntryT>;
    const auto &query_graph =
        static_cast<const common::WeightedGraph<ev::LimitedTradeoffFunction> &>(graph);
    using NodePotentials = common::ZeroNodePotentials<ev::TradeoffGraph>;
    return common::fp_dijkstra(start, target, ev::make_constant(0, 0), query_graph, queue, labels,
                               NodePotentials{}, Policy{x_eps, y_eps, capacity});
}

template <typename LabelEntryT, typename NodePotentialsT>
auto fp_astar(const ev::TradeoffGraph::node_id_t start, const ev::TradeoffGraph::node_id_t target,
              const ev::TradeoffGraph &graph, common::MinIDQueue &queue,
              common::NodeLabels<TradeoffDijkstraPolicy<LabelEntryT>> &labels,
              const NodePotentialsT &potentials,
              const double capacity = std::numeric_limits<double>::infinity(),
              const double x_eps = 0.1, const double y_eps = 1.0) {
    using Policy = TradeoffDijkstraPolicy<LabelEntryT>;
    const auto &query_graph =
        static_cast<const common::WeightedGraph<ev::LimitedTradeoffFunction> &>(graph);
    return common::fp_dijkstra(start, target, ev::make_constant(0, 0), query_graph, queue, labels,
                               potentials, Policy{x_eps, y_eps, capacity});
}

// Function propagation with Dijkstra
struct FPDijkstraContext {
    FPDijkstraContext(const double x_eps, const double y_eps, const double capacity,
                      const TradeoffGraph &graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), graph(graph), queue(graph.num_nodes()),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPDijkstraContext(FPDijkstraContext &&) = default;
    FPDijkstraContext(const FPDijkstraContext &) = default;
    FPDijkstraContext &operator=(FPDijkstraContext &&) = default;
    FPDijkstraContext &operator=(const FPDijkstraContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        return fp_dijkstra(start, target, graph, queue, labels, capacity, x_eps, y_eps);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const TradeoffGraph &graph;
    common::MinIDQueue queue;
    common::NodeLabels<TradeoffDijkstraPolicyWithParents> labels;
};

// Function propagation with A*
struct FPAStarContext {
    FPAStarContext(const double x_eps, const double y_eps, const double capacity,
                   const TradeoffGraph &graph, const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), graph(graph), queue(graph.num_nodes()),
          potentials(reverse_min_duration_graph), labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPAStarContext(FPAStarContext &&) = default;
    FPAStarContext(const FPAStarContext &) = default;
    FPAStarContext &operator=(FPAStarContext &&) = default;
    FPAStarContext &operator=(const FPAStarContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return fp_astar(start, target, graph, queue, labels, potentials, capacity, x_eps, y_eps);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const TradeoffGraph &graph;
    common::MinIDQueue queue;
    common::LandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<TradeoffDijkstraPolicyWithParents> labels;
};

// Function propagation using chargers with A*
struct FPAStarOmegaContext {
    FPAStarOmegaContext(const double x_eps, const double y_eps, const double capacity,
                        const double min_tradeoff_rate, const TradeoffGraph &graph,
                        const DurationGraph &reverse_min_duration_graph,
                        const ConsumptionGraph &reverse_consumption_graph,
                        const OmegaGraph &reverse_omega_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), graph(graph), queue(graph.num_nodes()),
          potentials(capacity, min_tradeoff_rate, reverse_min_duration_graph,
                     reverse_consumption_graph, reverse_omega_graph),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    FPAStarOmegaContext(FPAStarOmegaContext &&) = default;
    FPAStarOmegaContext(const FPAStarOmegaContext &) = default;
    FPAStarOmegaContext &operator=(FPAStarOmegaContext &&) = default;
    FPAStarOmegaContext &operator=(const FPAStarOmegaContext &) = default;

    auto operator()(const TradeoffGraph::node_id_t start, const TradeoffGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return fp_astar(start, target, graph, queue, labels, potentials, capacity, x_eps, y_eps);
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const TradeoffGraph &graph;
    common::MinIDQueue queue;
    ev::OmegaNodePotentials potentials;
    common::NodeLabels<TradeoffDijkstraPolicyWithParents> labels;
};
}

#endif
