#ifndef CHARGE_COMMON_MULTI_CRITERIA_DIJKSTRA_HPP
#define CHARGE_COMMON_MULTI_CRITERIA_DIJKSTRA_HPP

#include "common/dijkstra_private.hpp"
#include "common/domination.hpp"
#include "common/id_queue.hpp"
#include "common/irange.hpp"
#include "common/lower_envelop.hpp"
#include "common/memory_statistics.hpp"
#include "common/node_label_container.hpp"
#include "common/node_potentials.hpp"
#include "common/options.hpp"
#include "common/sample_function.hpp"
#include "common/sink_iter.hpp"
#include "common/stateful_function.hpp"
#include "common/weighted_graph.hpp"

#include <iostream>
#include <thread>
#include <tuple>
#include <vector>

namespace charge::common {

// Implementation of above traits
namespace bi_criterial_traits {

template <typename T1, typename T2>
bool dominates(const std::tuple<T1, T2> &lhs, const std::tuple<T1, T2> &rhs) {
    return dominates_lexicographical(lhs, rhs);
}

template <typename T1, typename T2>
std::tuple<T1, T2> link(const std::tuple<T1, T2> &lhs, const std::tuple<T1, T2> &rhs) {
    return {std::get<0>(lhs) + std::get<0>(rhs), std::get<1>(lhs) + std::get<1>(rhs)};
}

template <typename OutIter>
auto link_compose(const std::tuple<std::int32_t, std::int32_t> &lhs,
                  const StatefulFunction<PiecewieseFunction<LinearFunction, inf_bound, clamp_bound,
                                                            monotone_decreasing>> &rhs,
                  double sample_resolution, OutIter out) {
    auto[lhs_x, lhs_y] = lhs;
    double y = from_fixed(lhs_y);
    double x = from_fixed(lhs_x);
    double min_x = rhs.inv_function(y);

    // we can't charge any high
    if (min_x >= rhs.function.max_x())
        return out;

    assert(std::isfinite(min_x));

    // converts from coordinate system of the stateful function
    // to the absolute coordinate system
    auto x_offset = common::to_fixed(x - min_x);

    constexpr double MIN_X_RESOLUTION = 0.01;

    const auto insert_sample = [&](const auto &sample) {
        const auto[sampled_x, sampled_y] = sample;
        const auto shifted_x = sampled_x + x_offset;
        // we are not allowed to go back in time
        assert(shifted_x >= x);
        *out++ = std::tuple<std::int32_t, std::int32_t>(shifted_x, sampled_y);
    };

    common::sample_function(min_x, rhs.function, MIN_X_RESOLUTION, sample_resolution,
                            common::make_sink_iter(std::ref(insert_sample)));
    return out;
}

template <typename T1, typename T2>
std::tuple<T1, T2> constrain_both(const std::tuple<T1, T2> &tuple,
                                  const std::tuple<T1, T2> &min_value,
                                  const std::tuple<T1, T2> &max_value) {
    if (std::get<0>(tuple) < std::get<0>(min_value))
        return {std::get<0>(min_value), std::get<1>(tuple)};
    else if (std::get<0>(tuple) > std::get<0>(max_value))
        return {INF_WEIGHT, INF_WEIGHT};
    else if (std::get<1>(tuple) < std::get<1>(min_value))
        return {std::get<0>(tuple), std::get<1>(min_value)};
    else if (std::get<1>(tuple) > std::get<1>(max_value))
        return {INF_WEIGHT, INF_WEIGHT};
    else
        return {std::get<0>(tuple), std::get<1>(tuple)};
}

template <typename T1, typename T2>
std::tuple<T1, T2> constrain_first(const std::tuple<T1, T2> &tuple,
                                   const std::tuple<T1, T2> &min_value,
                                   const std::tuple<T1, T2> &max_value) {
    if (std::get<0>(tuple) < std::get<0>(min_value))
        return {std::get<0>(min_value), std::get<1>(tuple)};
    else if (std::get<0>(tuple) > std::get<0>(max_value))
        return {INF_WEIGHT, INF_WEIGHT};
    else
        return {std::get<0>(tuple), std::get<1>(tuple)};
}

template <typename T1, typename T2>
std::tuple<T1, T2> constrain_second(const std::tuple<T1, T2> &tuple,
                                    const std::tuple<T1, T2> &min_value,
                                    const std::tuple<T1, T2> &max_value) {
    if (std::get<1>(tuple) < std::get<1>(min_value))
        return {std::get<0>(tuple), std::get<1>(min_value)};
    else if (std::get<1>(tuple) > std::get<1>(max_value))
        return {INF_WEIGHT, INF_WEIGHT};
    else
        return {std::get<0>(tuple), std::get<1>(tuple)};
}

template <typename T1, typename T2> auto first_entry(const std::tuple<T1, T2> &tuple) {
    return std::get<0>(tuple);
}

template <typename NodeLabelsT>
bool min_key_terminate(const MinIDQueue &queue, const NodeLabelsT &labels,
                       const typename NodeLabelsT::node_id_t target) {
    if (labels[target].empty())
        return false;

    return queue.peek().key > first_entry(labels[target].front().cost);
}
} // namespace bi_criterial_traits

namespace multi_criteria_traits {
// FIXME we don't have any implementation for more then two criteria
using namespace bi_criterial_traits;
} // namespace multi_criteria_traits

template <typename GraphT, typename NodeEntryT> class MCDijkstraPolicy {
  public:
    using graph_t = GraphT;
    using queue_t = MinIDQueue;
    using key_t = std::int32_t;
    using weight_t = typename GraphT::weight_t;
    using cost_t = typename GraphT::weight_t;
    using node_id_t = typename GraphT::node_id_t;
    using label_t = NodeEntryT;

    static constexpr bool enable_stalling = true;

    template <typename NodeLabelsT>
    static bool terminate(const MinIDQueue &, const NodeLabelsT &, const node_id_t) {
        return false;
    }

    static auto link(const cost_t &lhs, const weight_t &rhs) {
        return multi_criteria_traits::link(lhs, rhs);
    }

    bool constrain(cost_t &) const {
        // Unconstrained
        return false;
    }

    static bool dominates(const cost_t &lhs, const cost_t &rhs) {
        Statistics::get().count(StatisticsEvent::DOMINATION);
        return multi_criteria_traits::dominates(lhs, rhs);
    }

    template <typename Range> static bool dominates(const Range &lhs_range, const cost_t &rhs) {
        return std::get<0>(clip_dominated(lhs_range, rhs));
    }

    // Clipping does nothing for points
    static auto clip_dominated(const cost_t &lhs, const cost_t &rhs) {
        return std::make_tuple(dominates(lhs, rhs), false);
    }

    template <typename Range>
    static auto clip_dominated(const Range &lhs_range, const cost_t &rhs) {
        for (const auto &lhs : lhs_range) {
            if (dominates(lhs, rhs))
                return std::make_tuple(true, true);
        }
        return std::make_tuple(false, false);
    }

    static auto key(const cost_t &cost) { return multi_criteria_traits::first_entry(cost); }
};

template <typename GraphT, typename LabelEntryT, typename NodeWeightsT>
class MCDijkstraWeightedNodePolicy : public MCDijkstraPolicy<GraphT, LabelEntryT> {
  public:
    using Base = MCDijkstraPolicy<GraphT, LabelEntryT>;
    using node_id_t = typename Base::node_id_t;
    using cost_t = typename LabelEntryT::cost_t;

    MCDijkstraWeightedNodePolicy(const NodeWeightsT &node_weights, double sample_resolution = 10.0,
                                 std::int32_t node_weight_penalty = 0)
        : node_weights(node_weights), sample_resolution{sample_resolution},
          node_weight_penalty{node_weight_penalty} {}

    bool weighted(const node_id_t node) const { return node_weights.weighted(node); }

    template <typename OutIter>
    auto link_node(const cost_t &lhs, const node_id_t node, OutIter out) const {
        assert(node_weights.weighted(node));
        return common::multi_criteria_traits::link_compose(lhs, node_weights.weight(node),
                                                           sample_resolution, out);
    }

    const std::int32_t node_weight_penalty;

  protected:
    double sample_resolution;
    const NodeWeightsT &node_weights;
};

namespace detail {

template <typename PolicyT, typename NodePotentialsT>
void route_step(typename PolicyT::queue_t &queue, NodeLabels<PolicyT> &labels,
                const NodePotentialsT &potentials, const typename PolicyT::graph_t &graph,
                const typename PolicyT::node_id_t target, const PolicyT &policy) {
    const auto top = queue.peek();
    // special case we can run into in case set of
    // unsettled labels was emptied after a pop
    if (labels.empty(top.id)) {
        queue.pop();
        return;
    }

    const auto[top_label, top_entry_id] = labels.pop(top.id, policy, potentials);
    if (labels.empty(top.id)) {
        queue.pop();
    } else {
        // sync the queue key with the new min label key
        const auto top_key = labels.min(top.id).key;
        assert(top_key >= 0);

        const auto current_top_key = queue.get_key(top.id);
        assert(top_key >= current_top_key);

        if (top_key > current_top_key) {
            queue.increase_key(IDKeyPair{top.id, top_key});
        }
    }

    // clang-format off
    if constexpr(PolicyT::enable_stalling) {
        if (labels.dominated(target, top_label.cost, policy)) {
            Statistics::get().count(StatisticsEvent::DIJKSTRA_STALL);
            return;
        }
    }
    // clang-format on

    Statistics::get().max(StatisticsEvent::LABEL_MAX_NUM_UNSETTLED, labels.size(top.id));
    Statistics::get().max(StatisticsEvent::LABEL_MAX_NUM_SETTLED, labels[top.id].size());

#ifdef CHARGE_ENABLE_MEMORY_STATISTICS
    if (Options::get().tail_memory) {
        thread_local int memory_log_counter = 0;
        if (memory_log_counter++ > 1000000) {
            std::cerr << std::this_thread::get_id() << ":\n" << memory_statistics(labels);
            memory_log_counter = 0;
        }
    }
#endif

    // clang-format off
    if constexpr (has_node_weights<PolicyT>::value) {
        if (policy.weighted(top.id)) {
            bool pruned = false;
            if constexpr(is_parent_label<typename PolicyT::label_t>::value) {
                if (top.id == top_label.parent)
                {
                    Statistics::get().count(StatisticsEvent::DIJKSTRA_PARENT_PRUNE);
                    pruned = true;
                }
            }

            if (!pruned) {
                Statistics::get().count(StatisticsEvent::DIJKSTRA_NODE_WEIGHT);

                unsigned parent_entry = top_entry_id;
                const auto &parent_cost = top_label.cost;
                const auto insert_solution = [&](const auto &solution) {
                    auto tentative_cost = solution;
                    std::get<0>(tentative_cost) += policy.node_weight_penalty;
                    // applying the constrain can clip away the whole function
                    if (policy.constrain(tentative_cost)) {
                        Statistics::get().count(StatisticsEvent::DIJKSTRA_CONTRAINT_CLIP);
                    } else {
                        if (!policy.dominates(parent_cost, tentative_cost)) {
                            assert(std::get<0>(tentative_cost) >= std::get<0>(parent_cost));
                            assert(std::get<1>(tentative_cost) >= 0);
                            const auto label_key =
                                potentials.key(target, PolicyT::key(tentative_cost), tentative_cost);
                            insert_label(queue, labels, policy, potentials, top.id,
                                         {label_key, std::move(tentative_cost), top.id, parent_entry});
                        }
                    }
                };

                policy.link_node(top_label.cost, top.id, make_sink_iter(std::ref(insert_solution)));
                labels.cleanup_unsettled(top.id);
            }
        }
    }
    // clang-format on

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        const auto target = graph.target(edge);
        // clang-format off
        if constexpr(is_parent_label<typename PolicyT::label_t>::value) {
            if (target == top_label.parent) {
                Statistics::get().count(StatisticsEvent::DIJKSTRA_PARENT_PRUNE);
                continue;
            }
        }
        // clang-format on

        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);

        const auto weight = graph.weight(edge);
        auto tentative_cost = PolicyT::link(top_label.cost, weight);
        if (policy.constrain(tentative_cost)) {
            Statistics::get().count(StatisticsEvent::DIJKSTRA_CONTRAINT_CLIP);
            continue;
        }
        assert(std::get<0>(tentative_cost) >= std::get<0>(top_label.cost));
        assert(std::get<1>(tentative_cost) >= 0);
        const auto label_key = potentials.key(target, PolicyT::key(tentative_cost), tentative_cost);
        insert_label(
            queue, labels, policy, potentials, target,
            {label_key, std::move(tentative_cost), top.id, static_cast<unsigned>(top_entry_id)});
    }
}
} // namespace detail

template <typename PolicyT, typename NodePotentialsT>
auto mc_dijkstra(const typename PolicyT::node_id_t start, const typename PolicyT::node_id_t target,
                 const typename PolicyT::graph_t &graph, typename PolicyT::queue_t &queue,
                 NodeLabels<PolicyT> &labels, const NodePotentialsT &potentials,
                 const PolicyT &policy) {
    queue.clear();
    labels.clear();

    labels.push(start, {0, typename PolicyT::cost_t{}, INVALID_ID, INVALID_ID}, policy, potentials);
    queue.push(IDKeyPair{start, 0});

    while (!queue.empty()) {
        if (PolicyT::terminate(queue, labels, target)) {
            break;
        }
        detail::route_step(queue, labels, potentials, graph, target, policy);
    }

    auto solutions = lower_envelop(labels[target]);
    return solutions;
}

template <typename PolicyT>
auto mc_dijkstra(const typename PolicyT::node_id_t start, const typename PolicyT::node_id_t target,
                 const typename PolicyT::graph_t &graph, typename PolicyT::queue_t &queue,
                 NodeLabels<PolicyT> &labels, const PolicyT &policy) {
    return mc_dijkstra(start, target, graph, queue, labels,
                       ZeroNodePotentials<typename PolicyT::graph_t>{}, policy);
}

template <typename LabelEntryT, typename GraphT>
auto mc_dijkstra(const typename GraphT::node_id_t start, const typename GraphT::node_id_t target,
                 const GraphT &graph, MinIDQueue &queue,
                 NodeLabels<MCDijkstraPolicy<GraphT, LabelEntryT>> &labels) {
    return mc_dijkstra(start, target, graph, queue, labels, ZeroNodePotentials<GraphT>{},
                       MCDijkstraPolicy<GraphT, LabelEntryT>{});
}

template <typename LabelEntryT, typename GraphT, typename NodeWeightsT>
auto mc_dijkstra(
    const typename GraphT::node_id_t start, const typename GraphT::node_id_t target,
    const GraphT &graph, NodeWeightsT &node_weights, MinIDQueue &queue,
    NodeLabels<MCDijkstraWeightedNodePolicy<GraphT, LabelEntryT, NodeWeightsT>> &labels) {
    return mc_dijkstra(
        start, target, graph, queue, labels, ZeroNodePotentials<GraphT>{},
        MCDijkstraWeightedNodePolicy<GraphT, LabelEntryT, NodeWeightsT>{node_weights});
}
} // namespace charge::common

#endif
