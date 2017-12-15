#ifndef CHARGE_COMMON_FP_DIJKSTRA_HPP
#define CHARGE_COMMON_FP_DIJKSTRA_HPP

#include "common/dijkstra_private.hpp"
#include "common/domination.hpp"
#include "common/limited_function.hpp"
#include "common/memory_statistics.hpp"
#include "common/minimize_combined_function.hpp"
#include "common/minimize_composed_function.hpp"
#include "common/node_potentials.hpp"
#include "common/options.hpp"

#include <thread>

namespace charge::common {

namespace function_propergation_traits {

inline auto link_combine(const PiecewieseDecHypOrLinFunction &lhs,
                         const LimitedHypOrLinFunction &rhs) {
    return combine_minimal(lhs, rhs);
}

template <typename OutIter>
inline auto link_compose(
    const PiecewieseDecHypOrLinFunction &lhs,
    const StatefulFunction<
        PiecewieseFunction<LinearFunction, inf_bound, clamp_bound, monotone_decreasing>> &rhs,
    OutIter out) {
    return compose_minimal(lhs, rhs, out);
}

inline auto dominates(const PiecewieseDecHypOrLinFunction &lhs,
                      const PiecewieseDecHypOrLinFunction &rhs, const std::uint32_t x_epsilon = 1,
                      const std::uint32_t y_epsilon = 0) {
    return epsilon_dominates_piecewiese(lhs, rhs, x_epsilon, y_epsilon);
}

template <typename Range>
inline auto dominates(const Range &lhs_range, const PiecewieseDecHypOrLinFunction &rhs,
                      const std::uint32_t x_epsilon = 1, const std::uint32_t y_epsilon = 0) {
    auto[begin_iter, end_iter] = find_undominated_range(
        lhs_range.begin(), lhs_range.end(), rhs.begin(), rhs.end(), x_epsilon, y_epsilon);
    return begin_iter == end_iter;
}

inline std::tuple<bool, bool> clip_dominated(const PiecewieseDecHypOrLinFunction &lhs,
                                             PiecewieseDecHypOrLinFunction &rhs,
                                             const std::uint32_t x_epsilon = 1,
                                             const std::uint32_t y_epsilon = 0) {
    auto begin_iter = rhs.functions.begin();
    auto end_iter = rhs.functions.end();

    begin_iter = find_first_undominated(lhs.functions.begin(), lhs.functions.end(), begin_iter,
                                        end_iter, x_epsilon, y_epsilon);
    end_iter = find_last_undominated(lhs.functions.begin(), lhs.functions.end(), begin_iter,
                                     end_iter, x_epsilon, y_epsilon);

    if (begin_iter == end_iter) {
        rhs.functions.clear();
        return std::make_tuple(true, true);
    }
    assert(std::distance(begin_iter, end_iter) >= 0);

    bool modified = false;
    if (begin_iter != rhs.functions.begin()) {
        end_iter = std::copy(begin_iter, end_iter, rhs.functions.begin());
        modified = true;
    }

    if (end_iter != rhs.functions.end()) {
        rhs.functions.resize(end_iter - rhs.functions.begin());
        modified = true;
    }

    return std::make_tuple(false, modified);
}

template <typename Range>
inline std::tuple<bool, bool>
clip_dominated(const Range &lhs_range, PiecewieseDecHypOrLinFunction &rhs,
               const std::uint32_t x_epsilon = 1, const std::uint32_t y_epsilon = 0) {
    auto[begin_iter, end_iter] = find_undominated_range(
        lhs_range.begin(), lhs_range.end(), rhs.begin(), rhs.end(), x_epsilon, y_epsilon);
    if (begin_iter == end_iter) {
        rhs.functions.clear();
        return std::make_tuple(true, true);
    }
    assert(std::distance(begin_iter, end_iter) > 0);

    bool modified = false;
    if (begin_iter != rhs.functions.begin()) {
        end_iter = std::copy(begin_iter, end_iter, rhs.functions.begin());
        modified = true;
    }

    if (end_iter != rhs.functions.end()) {
        rhs.functions.resize(end_iter - rhs.functions.begin());
        modified = true;
    }

    return std::make_tuple(false, modified);
}

inline auto key(const PiecewieseDecHypOrLinFunction &cost) { return to_fixed(cost.min_x()); }

template <typename NodeLabelsT>
bool min_key_terminate(const MinIDQueue &queue, const NodeLabelsT &labels,
                       const typename NodeLabelsT::node_id_t target) {
    if (labels[target].empty())
        return false;

    return queue.peek().key > key(labels[target].front().cost) + common::to_fixed(0.1);
}
} // namespace function_propergation_traits

template <typename GraphT, typename LabelEntryT> class FPDijkstraPolicy;
template <typename GraphT, typename LabelEntryT, typename NodeWeightsT>
class FPDijkstraWeightedNodePolicy;

using HypLinGraph = WeightedGraph<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>>;

template <typename LabelEntryT> class FPDijkstraPolicy<HypLinGraph, LabelEntryT> {
  public:
    using graph_t = WeightedGraph<LimitedHypOrLinFunction>;
    using label_t = LabelEntryT;
    using cost_t = typename label_t::cost_t;
    using weight_t = typename graph_t::weight_t;
    using node_id_t = typename graph_t::node_id_t;
    using queue_t = MinIDQueue;
    using key_t = std::uint32_t;

    static constexpr bool enable_stalling = true;

    template <typename NodeLabelsT>
    static bool terminate(const MinIDQueue &, const NodeLabelsT &, const node_id_t) {
        return false;
    }

    static auto link(const cost_t &lhs, const weight_t &rhs) {
        return function_propergation_traits::link_combine(lhs, rhs);
    }

    // Unconstrained
    bool constrain(const cost_t &cost) const { return cost.functions.empty(); }

    static key_t key(const cost_t &cost) { return function_propergation_traits::key(cost); }

    bool dominates(const cost_t &lhs, const cost_t &rhs) const {
        Statistics::get().count(StatisticsEvent::DOMINATION);
        return function_propergation_traits::dominates(lhs, rhs);
    }

    template <typename Range> bool dominates(const Range &lhs_range, const cost_t &rhs) const {
        Statistics::get().count(StatisticsEvent::DOMINATION);
        return function_propergation_traits::dominates(lhs_range, rhs);
    }

    auto clip_dominated(const cost_t &lhs, cost_t &rhs) const {
        Statistics::get().count(StatisticsEvent::DOMINATION);
        return function_propergation_traits::clip_dominated(lhs, rhs);
    }

    template <typename Range> auto clip_dominated(const Range &lhs_range, cost_t &rhs) const {
        Statistics::get().count(StatisticsEvent::DOMINATION);
        return function_propergation_traits::clip_dominated(lhs_range, rhs);
    }
};

template <typename LabelEntryT, typename NodeWeightsT>
class FPDijkstraWeightedNodePolicy<HypLinGraph, LabelEntryT, NodeWeightsT>
    : public FPDijkstraPolicy<HypLinGraph, LabelEntryT> {
  public:
    using Base = FPDijkstraPolicy<HypLinGraph, LabelEntryT>;
    using node_id_t = typename Base::node_id_t;
    using cost_t = typename LabelEntryT::cost_t;

    FPDijkstraWeightedNodePolicy(const NodeWeightsT &node_weights,
                                 const double node_weight_penalty = 0)
        : node_weights(node_weights), node_weight_penalty{node_weight_penalty} {}

    bool weighted(const node_id_t node) const { return node_weights.weighted(node); }

    template <typename OutIter>
    auto link_node(const cost_t &lhs, const node_id_t node, OutIter out) const {
        assert(node_weights.weighted(node));
        return common::function_propergation_traits::link_compose(lhs, node_weights.weight(node),
                                                                  out);
    }

    const double node_weight_penalty;

  protected:
    const NodeWeightsT &node_weights;
};

namespace detail {
template <typename FnT, typename PolicyT, typename NodePotentialsT>
void route_step(typename PolicyT::queue_t &queue, NodeLabels<PolicyT> &labels,
                const NodePotentialsT &potentials, const FunctionGraph<FnT> &graph,
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
        assert(top_key > 0);

        const auto current_top_key = queue.get_key(top.id);
        // we need to account for slight rounding errors
        assert(top_key + common::to_fixed(0.001) >= current_top_key);

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
    if constexpr (has_node_weights<PolicyT>::value)
    {
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
                const auto insert_solution = [&](PiecewieseSolution &&solution) {
                    auto[delta, tentative_cost] = std::move(solution);
                    // applying the constrain can clip away the whole function
                    if (policy.constrain(tentative_cost)) {
                        Statistics::get().count(StatisticsEvent::DIJKSTRA_CONTRAINT_CLIP);
                    } else {
                        Statistics::get().max(StatisticsEvent::LABEL_MAX_LENGTH,
                                              tentative_cost.functions.size());
                        Statistics::get().sum(StatisticsEvent::LABEL_SUM_LENGTH,
                                              tentative_cost.functions.size());
                        tentative_cost.shift(policy.node_weight_penalty);
                        delta.shift(policy.node_weight_penalty);
                        const auto label_key = potentials.key(target, PolicyT::key(tentative_cost), tentative_cost);
                        insert_label(
                            queue, labels, policy, potentials, top.id,
                            {label_key, std::move(tentative_cost), std::move(delta), top.id, parent_entry});
                    }
                };

                policy.link_node(top_label.cost, top.id, make_sink_iter(std::ref(insert_solution)));
            }
        }
    }
    // clang-format on

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);
        const auto target = graph.target(edge);

        // clang-format off
        if constexpr(is_parent_label<typename PolicyT::label_t>::value) {
            if (target == top_label.parent)
            {
                Statistics::get().count(StatisticsEvent::DIJKSTRA_PARENT_PRUNE);
                continue;
            }
        }
        // clang-format on

        const auto &weight = graph.weight(edge);
        assert(potentials.check_consitency(top.id, target, weight));
        auto[delta, tentative_cost] = PolicyT::link(top_label.cost, weight);

        // applying the constrain can clip away the whole function
        if (policy.constrain(tentative_cost)) {
            Statistics::get().count(StatisticsEvent::DIJKSTRA_CONTRAINT_CLIP);
        } else {
            Statistics::get().max(StatisticsEvent::LABEL_MAX_LENGTH, tentative_cost.size());
            Statistics::get().sum(StatisticsEvent::LABEL_SUM_LENGTH, tentative_cost.size());
            Statistics::get().max(StatisticsEvent::LABEL_DELTA_MAX_LENGTH, delta.size());
            Statistics::get().sum(StatisticsEvent::LABEL_DELTA_SUM_LENGTH, delta.size());
            const auto label_key =
                potentials.key(target, PolicyT::key(tentative_cost), tentative_cost);
            insert_label(queue, labels, policy, potentials, target,
                         {label_key, std::move(tentative_cost), std::move(delta), top.id,
                          static_cast<unsigned>(top_entry_id)});
        }
    }
}
} // namespace detail

template <typename PolicyT, typename NodePotentialsT>
auto fp_dijkstra(const typename PolicyT::node_id_t start, const typename PolicyT::node_id_t target,
                 const typename PolicyT::weight_t start_weight,
                 const typename PolicyT::graph_t &graph, typename PolicyT::queue_t &queue,
                 NodeLabels<PolicyT> &labels, const NodePotentialsT &potentials,
                 const PolicyT &policy) {
    queue.clear();
    labels.clear();

    using label_t = typename PolicyT::label_t;
    labels.push(
        start,
        label_t{0, PiecewieseDecHypOrLinFunction{{start_weight}},
                InterpolatingIncFunction{{LimitedLinearFunction{0, 0, LinearFunction(0, 0)}}},
                INVALID_ID, INVALID_ID},
        policy, potentials);
    queue.push(IDKeyPair{start, 0});

    while (!queue.empty()) {
        if (PolicyT::terminate(queue, labels, target)) {
            break;
        }
        detail::route_step(queue, labels, potentials, graph, target, policy);
    }

    auto result = labels[target];
    std::sort(result.begin(), result.end());
    return result;
}

template <typename PolicyT>
auto fp_dijkstra(const typename PolicyT::node_id_t start, const typename PolicyT::node_id_t target,
                 const typename PolicyT::graph_t &graph, typename PolicyT::queue_t &queue,
                 NodeLabels<PolicyT> &labels, const PolicyT &policy) {
    using NodePotentialsT = ZeroNodePotentials<typename PolicyT::graph_t>;
    return fp_dijkstra(start, target, typename PolicyT::weight_t{}, graph, queue, labels,
                       NodePotentialsT{}, policy);
}

template <typename LabelEntryT, typename FnT>
auto fp_dijkstra(const typename FunctionGraph<FnT>::node_id_t start,
                 const typename FunctionGraph<FnT>::node_id_t target,
                 const FunctionGraph<FnT> &graph, MinIDQueue &queue,
                 NodeLabels<FPDijkstraPolicy<FunctionGraph<FnT>, LabelEntryT>> &labels) {
    using GraphT = FunctionGraph<FnT>;
    using Policy = FPDijkstraPolicy<GraphT, LabelEntryT>;
    using NodePotentialsT = ZeroNodePotentials<GraphT>;
    return fp_dijkstra(start, target, typename Policy::weight_t{0, 0, FnT{}}, graph, queue, labels,
                       NodePotentialsT{}, Policy{});
}

template <typename LabelEntryT, typename FnT, typename NodeWeightsT>
auto fp_dijkstra(
    const typename FunctionGraph<FnT>::node_id_t start,
    const typename FunctionGraph<FnT>::node_id_t target, const FunctionGraph<FnT> &graph,
    const NodeWeightsT &node_weights, MinIDQueue &queue,
    NodeLabels<FPDijkstraWeightedNodePolicy<FunctionGraph<FnT>, LabelEntryT, NodeWeightsT>>
        &labels) {
    using GraphT = FunctionGraph<FnT>;
    using Policy = FPDijkstraWeightedNodePolicy<FunctionGraph<FnT>, LabelEntryT, NodeWeightsT>;
    using NodePotentialsT = ZeroNodePotentials<GraphT>;

    return fp_dijkstra(start, target, typename Policy::weight_t{0, 0, FnT{}}, graph, queue, labels,
                       NodePotentialsT{}, Policy{node_weights});
}
} // namespace charge::common

#endif
