#ifndef CHARGE_EV_MC_DIJKSTRA_HPP
#define CHARGE_EV_MC_DIJKSTRA_HPP

#include "ev/graph.hpp"

#include "common/domination.hpp"
#include "common/mc_dijkstra.hpp"
#include "common/statistics.hpp"

namespace charge::ev {

// Tuned to work well with a metric consitsing of deci-seconds and deci-mWh
// Capacity is in deci-mWh and by default unrestricted
template <typename LabelEntryT>
class DurationConsumptionDijkstraPolicy
    : public common::MCDijkstraPolicy<DurationConsumptionGraph, LabelEntryT> {
  public:
    using Base = common::MCDijkstraPolicy<DurationConsumptionGraph, LabelEntryT>;

    using typename Base::node_id_t;
    using typename Base::cost_t;

    DurationConsumptionDijkstraPolicy(const std::int32_t capacity)
        : DurationConsumptionDijkstraPolicy(common::to_fixed(0.1), common::to_fixed(1), capacity) {}

    DurationConsumptionDijkstraPolicy(const std::int32_t x_epsilon, const std::int32_t y_epsilon,
                                      const std::int32_t capacity)
        : x_epsilon(x_epsilon), y_epsilon(y_epsilon), capacity(capacity) {}

    template <typename NodeLabelsT>
    static bool terminate(const common::MinIDQueue &queue, const NodeLabelsT &labels,
                          const node_id_t target) {
        return common::bi_criterial_traits::min_key_terminate(queue, labels, target);
    }

    bool constrain(cost_t &cost) const {
        assert(std::get<0>(cost) >= 0);
        // applies battery constraints
        cost = common::bi_criterial_traits::constrain_second(cost, {0, 0},
                                                             {common::INF_WEIGHT, capacity});
        return std::get<0>(cost) == common::INF_WEIGHT;
    }

    auto dominates(const cost_t &lhs, const cost_t &rhs) const {
        common::Statistics::get().count(common::StatisticsEvent::DOMINATION);
        return common::epsilon_dominates_lexicographical(lhs, rhs, x_epsilon, y_epsilon);
    }

    template <typename Range> bool dominates(const Range &lhs_range, const cost_t &rhs) const {
        return std::get<0>(clip_dominated(lhs_range, rhs));
    }

    // Clipping does nothing for points
    auto clip_dominated(const cost_t &lhs, const cost_t &rhs) const { return std::make_tuple(dominates(lhs, rhs), false); }

    template <typename Range> auto clip_dominated(const Range &lhs_range, const cost_t &rhs) const {
        for (const auto &lhs : lhs_range) {
            if (dominates(lhs, rhs))
                return std::make_tuple(true, true);
        }
        return std::make_tuple(false, false);
    }

    const std::int32_t x_epsilon;
    const std::int32_t y_epsilon;
    const std::int32_t capacity;
};

using DurationConsumptionLabelEntryWithParent =
    common::LabelEntryWithParent<DurationConsumptionGraph::weight_t,
                                 DurationConsumptionGraph::node_id_t>;
using DurationConsumptionDijkstraPolicyWithParents =
    DurationConsumptionDijkstraPolicy<DurationConsumptionLabelEntryWithParent>;

// Runs a Multi-Criteria dijkstra with Pareto-Dominanz on a bi-criterial graph
template <typename LabelEntryT>
auto mc_dijkstra(const DurationConsumptionGraph::node_id_t start,
                 const DurationConsumptionGraph::node_id_t target,
                 const DurationConsumptionGraph &graph, common::MinIDQueue &queue,
                 common::NodeLabels<DurationConsumptionDijkstraPolicy<LabelEntryT>> &labels,
                 const std::int32_t capacity = common::INF_WEIGHT,
                 const std::int32_t x_eps = common::to_fixed(0.1),
                 const std::int32_t y_eps = common::to_fixed(1.0)) {
    using Policy = DurationConsumptionDijkstraPolicy<LabelEntryT>;
    return common::mc_dijkstra(start, target, graph, queue, labels,
                               common::ZeroNodePotentials<DurationConsumptionGraph>{},
                               Policy{x_eps, y_eps, capacity});
}

template <typename LabelEntryT, typename NodePotentialT>
auto mc_astar(const DurationConsumptionGraph::node_id_t start,
              const DurationConsumptionGraph::node_id_t target,
              const DurationConsumptionGraph &graph, common::MinIDQueue &queue,
              common::NodeLabels<DurationConsumptionDijkstraPolicy<LabelEntryT>> &labels,
              const NodePotentialT &potentials, const std::int32_t capacity = common::INF_WEIGHT,
              const std::int32_t x_eps = common::to_fixed(0.1),
              const std::int32_t y_eps = common::to_fixed(1.0)) {
    using Policy = DurationConsumptionDijkstraPolicy<LabelEntryT>;
    return common::mc_dijkstra(start, target, graph, queue, labels, potentials,
                               Policy{x_eps, y_eps, capacity});
}

// Multi-Criteria with Dijkstra
struct MCDijkstraContext {
    MCDijkstraContext(const double x_eps, const double y_eps, const double capacity,
                      const DurationConsumptionGraph &graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), graph(graph), queue(graph.num_nodes()),
          labels(graph.num_nodes()) {}

    // Make copyable and movable
    MCDijkstraContext(MCDijkstraContext &&) = default;
    MCDijkstraContext(const MCDijkstraContext &) = default;
    MCDijkstraContext &operator=(MCDijkstraContext &&) = default;
    MCDijkstraContext &operator=(const MCDijkstraContext &) = default;

    auto operator()(const DurationConsumptionGraph::node_id_t start,
                    const DurationConsumptionGraph::node_id_t target) {
        return mc_dijkstra(start, target, graph, queue, labels, common::to_fixed(capacity),
                           common::to_fixed(x_eps), common::to_fixed(y_eps));
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const DurationConsumptionGraph &graph;
    common::MinIDQueue queue;
    common::NodeLabels<DurationConsumptionDijkstraPolicyWithParents> labels;
};

// Multi-Criteria with A*
struct MCAStarContext {
    MCAStarContext(const double x_eps, const double y_eps, const double capacity,
                   const DurationConsumptionGraph &graph,
                   const DurationGraph &reverse_min_duration_graph)
        : x_eps(x_eps), y_eps(y_eps), capacity(capacity), graph(graph), queue(graph.num_nodes()),
          potentials(reverse_min_duration_graph), labels(graph.num_nodes()) {}

    // Make copyable and movable
    MCAStarContext(MCAStarContext &&) = default;
    MCAStarContext(const MCAStarContext &) = default;
    MCAStarContext &operator=(MCAStarContext &&) = default;
    MCAStarContext &operator=(const MCAStarContext &) = default;

    auto operator()(const DurationConsumptionGraph::node_id_t start,
                    const DurationConsumptionGraph::node_id_t target) {
        potentials.recompute(queue, target);
        return mc_astar(start, target, graph, queue, labels, potentials, common::to_fixed(capacity),
                        common::to_fixed(x_eps), common::to_fixed(y_eps));
    }

    const double x_eps;
    const double y_eps;
    const double capacity;
    const DurationConsumptionGraph &graph;
    common::MinIDQueue queue;
    common::LandmarkNodePotentials<DurationGraph> potentials;
    common::NodeLabels<DurationConsumptionDijkstraPolicyWithParents> labels;
};
}

#endif
