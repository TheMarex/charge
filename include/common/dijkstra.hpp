#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP

#include "common/id_queue.hpp"
#include "common/lazy_clear_vector.hpp"
#include "common/weighted_graph.hpp"

namespace charge {
namespace common {

template <typename GraphT> using CostVector = LazyClearVector<typename GraphT::weight_t>;
template <typename GraphT> using ParentVector = LazyClearVector<typename GraphT::node_id_t>;

template <typename GraphT>
inline bool terminate_sum_min(MinIDQueue &forward_queue, MinIDQueue &reverse_queue,
                              const typename GraphT::weight_t best_cost) {
    return !forward_queue.empty() && !reverse_queue.empty() &&
           forward_queue.peek().key + reverse_queue.peek().key >= best_cost;
}

template <typename GraphT>
inline bool terminate_queues_empty(MinIDQueue &, MinIDQueue &, const typename GraphT::weight_t) {
    return false;
}

template <typename GraphT>
inline bool terminate_key_min(MinIDQueue &queue, const typename GraphT::weight_t target_cost) {
    return !queue.empty() && queue.peek().key >= target_cost;
}

template <typename CostT> inline CostT unconstrained(const CostT cost) { return cost; }

template <typename GraphT> inline bool no_stall(const typename GraphT::node_id_t) { return false; }

namespace detail {
template <typename GraphT, typename StallFn>
void route_step(MinIDQueue &queue, CostVector<GraphT> &forward_costs,
                CostVector<GraphT> &reverse_costs, const GraphT &graph,
                typename GraphT::node_id_t &middle, typename GraphT::weight_t &best_cost,
                StallFn stall) {
    auto top = queue.pop();
    auto cost_to_top = top.key;

    if (stall(top.id)) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_STALL);
        return;
    }

    if (reverse_costs.peek(top.id) < INF_WEIGHT) {
        auto tentative_best_cost = cost_to_top + reverse_costs.peek(top.id);
        if (tentative_best_cost < best_cost) {
            middle = top.id;
            best_cost = tentative_best_cost;
        }
    }

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);
        auto target = graph.target(edge);
        auto weight = graph.weight(edge);
        auto tentative_cost = cost_to_top + weight;

        if (tentative_cost < forward_costs.peek(target)) {
            if (queue.contains_id(target)) {
                queue.decrease_key(IDKeyPair{target, tentative_cost});
            } else {
                queue.push(IDKeyPair{target, tentative_cost});
            }

            forward_costs[target] = tentative_cost;
        }
    }
}

template <typename GraphT, typename StallFn>
void route_step(MinIDQueue &queue, CostVector<GraphT> &forward_costs,
                CostVector<GraphT> &reverse_costs, ParentVector<GraphT> &forward_parents,
                const GraphT &graph, typename GraphT::node_id_t &middle,
                typename GraphT::weight_t &best_cost, StallFn stall) {
    auto top = queue.pop();
    auto cost_to_top = top.key;

    if (stall(top.id)) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_STALL);
        return;
    }

    if (reverse_costs.peek(top.id) < INF_WEIGHT) {
        auto tentative_best_cost = cost_to_top + reverse_costs.peek(top.id);
        if (tentative_best_cost < best_cost) {
            middle = top.id;
            best_cost = tentative_best_cost;
        }
    }

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);
        auto target = graph.target(edge);
        auto weight = graph.weight(edge);
        auto tentative_cost = cost_to_top + weight;

        if (tentative_cost < forward_costs.peek(target)) {
            if (queue.contains_id(target)) {
                queue.decrease_key(IDKeyPair{target, tentative_cost});
            } else {
                queue.push(IDKeyPair{target, tentative_cost});
            }

            forward_costs[target] = tentative_cost;
            forward_parents[target] = top.id;
        }
    }
}

template <typename GraphT, typename ConstrainFn>
void constrained_route_step(MinIDQueue &queue, CostVector<GraphT> &costs, const GraphT &graph,
                            ConstrainFn constrain) {
    auto top = queue.pop();
    auto cost_to_top = top.key;

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);
        auto target = graph.target(edge);
        auto weight = graph.weight(edge);
        auto tentative_cost = constrain(cost_to_top + weight);

        if (tentative_cost < costs.peek(target)) {
            if (queue.contains_id(target)) {
                queue.decrease_key(IDKeyPair{target, tentative_cost});
            } else {
                queue.push(IDKeyPair{target, tentative_cost});
            }

            costs[target] = tentative_cost;
        }
    }
}

template <typename GraphT>
auto route_step(MinIDQueue &queue, CostVector<GraphT> &costs, const GraphT &graph) {
    return constrained_route_step(queue, costs, graph, unconstrained<typename GraphT::weight_t>);
}

template <typename GraphT>
void route_step(MinIDQueue &queue, CostVector<GraphT> &costs, ParentVector<GraphT> &parents,
                const GraphT &graph) {
    auto top = queue.pop();
    auto cost_to_top = top.key;

    for (auto edge = graph.begin(top.id); edge < graph.end(top.id); ++edge) {
        Statistics::get().count(StatisticsEvent::DIJKSTRA_RELAX);
        auto target = graph.target(edge);
        auto weight = graph.weight(edge);
        auto tentative_cost = cost_to_top + weight;

        if (tentative_cost < costs.peek(target)) {
            if (queue.contains_id(target)) {
                queue.decrease_key(IDKeyPair{target, tentative_cost});
            } else {
                queue.push(IDKeyPair{target, tentative_cost});
            }

            costs[target] = tentative_cost;
            parents[target] = top.id;
        }
    }
}
} // namespace detail

template <typename GraphT, typename TerminateFn, typename StallFn>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &forward_graph, const GraphT &reverse_graph, MinIDQueue &forward_queue,
              MinIDQueue &reverse_queue, CostVector<GraphT> &forward_costs,
              CostVector<GraphT> &reverse_costs, TerminateFn terminate, StallFn stall) {
    forward_costs.clear();
    reverse_costs.clear();
    forward_queue.clear();
    reverse_queue.clear();
    forward_queue.push(IDKeyPair{start, 0});
    reverse_queue.push(IDKeyPair{target, 0});
    forward_costs[start] = 0;
    reverse_costs[target] = 0;

    typename GraphT::node_id_t middle = INVALID_ID;
    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!forward_queue.empty() || !reverse_queue.empty()) {
        if (!forward_queue.empty()) {
            detail::route_step(forward_queue, forward_costs, reverse_costs, forward_graph, middle,
                               best_cost, stall);
        }

        if (!reverse_queue.empty()) {
            detail::route_step(reverse_queue, reverse_costs, forward_costs, reverse_graph, middle,
                               best_cost, stall);
        }

        if (terminate(forward_queue, reverse_queue, best_cost)) {
            return best_cost;
        }
    }

    return best_cost;
}

template <typename GraphT, typename TerminateFn, typename StallFn>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &forward_graph, const GraphT &reverse_graph, MinIDQueue &forward_queue,
              MinIDQueue &reverse_queue, CostVector<GraphT> &forward_costs,
              CostVector<GraphT> &reverse_costs, ParentVector<GraphT> &forward_parents,
              ParentVector<GraphT> &backward_parents, typename GraphT::node_id_t &middle,
              TerminateFn terminate, StallFn stall) {
    forward_costs.clear();
    reverse_costs.clear();
    forward_parents.clear();
    backward_parents.clear();
    forward_queue.clear();
    reverse_queue.clear();
    forward_queue.push(IDKeyPair{start, 0});
    reverse_queue.push(IDKeyPair{target, 0});
    forward_costs[start] = 0;
    reverse_costs[target] = 0;

    middle = INVALID_ID;
    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!forward_queue.empty() || !reverse_queue.empty()) {
        if (!forward_queue.empty()) {
            detail::route_step(forward_queue, forward_costs, reverse_costs, forward_parents,
                               forward_graph, middle, best_cost, stall);
        }

        if (!reverse_queue.empty()) {
            detail::route_step(reverse_queue, reverse_costs, forward_costs, backward_parents,
                               reverse_graph, middle, best_cost, stall);
        }

        if (terminate(forward_queue, reverse_queue, best_cost)) {
            return best_cost;
        }
    }

    return best_cost;
}

template <typename GraphT, typename TerminateFn>
auto dijkstra_to_all(typename GraphT::node_id_t start, const GraphT &graph, MinIDQueue &queue,
                     CostVector<GraphT> &costs, TerminateFn terminate) {
    costs.clear();
    queue.clear();
    queue.push(IDKeyPair{start, 0});
    costs[start] = 0;

    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!queue.empty()) {
        detail::route_step(queue, costs, graph);

        if (terminate(queue))
            return;
    }
}

template <typename GraphT, typename TerminateFn>
auto dijkstra_to_all(
    const std::vector<std::tuple<typename GraphT::node_id_t, typename GraphT::weight_t>> &sources,
    const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs, TerminateFn terminate) {
    costs.clear();
    queue.clear();

    for (const auto &s : sources) {
        auto[start, weight] = s;
        queue.push(IDKeyPair{start, weight});
        costs[start] = weight;
    }

    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!queue.empty()) {
        detail::route_step(queue, costs, graph);

        if (terminate(queue))
            return;
    }
}

template <typename GraphT>
auto continue_dijkstra(typename GraphT::node_id_t target, const GraphT &graph, MinIDQueue &queue,
                       CostVector<GraphT> &costs, std::vector<bool> &settled) {
    while (!queue.empty() && !settled[target]) {
        const auto id = queue.peek().id;
        settled[id] = true;

        detail::route_step(queue, costs, graph);
    }

    return costs[target];
}

template <typename GraphT>
auto dijkstra(typename GraphT::node_id_t source, typename GraphT::node_id_t target,
              const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs,
              std::vector<bool> &settled) {
    queue.clear();
    costs.clear();
    std::fill(settled.begin(), settled.end(), false);
    costs[source] = 0;
    queue.push({source, 0});

    while (!queue.empty()) {
        auto id = queue.peek().id;
        settled[id] = true;

        detail::route_step(queue, costs, graph);

        if (id == target)
        {
            break;
        }
    }

    return costs[target];
}

template <typename GraphT, typename TerminateFn>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs,
              ParentVector<GraphT> &parents, TerminateFn terminate) {
    costs.clear();
    queue.clear();
    parents.clear();
    queue.push(IDKeyPair{start, 0});
    costs[start] = 0;

    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!queue.empty()) {
        detail::route_step(queue, costs, parents, graph);

        if (terminate(queue, costs[target]))
            return costs[target];
    }

    return costs[target];
}
template <typename GraphT, typename TerminateFn, typename ConstrainFn>
auto constrained_dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
                          const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs,
                          TerminateFn terminate, ConstrainFn constrain) {
    costs.clear();
    queue.clear();
    queue.push(IDKeyPair{start, 0});
    costs[start] = 0;

    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!queue.empty()) {
        detail::constrained_route_step(queue, costs, graph, constrain);

        if (terminate(queue))
            return costs[target];
    }

    return costs[target];
}

template <typename GraphT, typename TerminateFn>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs,
              TerminateFn terminate) {
    return dijkstra(start, target, graph, queue, costs, terminate,
                    unconstrained<typename GraphT::weight_t>);
}

template <typename GraphT, typename TerminateFn, typename ConstrainFn>
auto constrained_dijkstra(
    const std::vector<std::tuple<typename GraphT::node_id_t, typename GraphT::weight_t>> &sources,
    typename GraphT::node_id_t target, const GraphT &graph, MinIDQueue &queue,
    CostVector<GraphT> &costs, TerminateFn terminate, ConstrainFn constrain) {
    costs.clear();
    queue.clear();
    for (const auto &s : sources) {
        auto[start, weight] = s;
        queue.push(IDKeyPair{start, weight});
        costs[start] = weight;
    }

    typename GraphT::weight_t best_cost = INF_WEIGHT;

    while (!queue.empty()) {
        detail::constrained_route_step(queue, costs, graph, constrain);

        if (terminate(queue))
            return costs[target];
    }

    return costs[target];
}

template <typename GraphT, typename TerminateFn>
auto dijkstra(
    const std::vector<std::tuple<typename GraphT::node_id_t, typename GraphT::weight_t>> &sources,
    typename GraphT::node_id_t target, const GraphT &graph, MinIDQueue &queue,
    CostVector<GraphT> &costs, TerminateFn terminate) {
    return dijkstra(sources, target, graph, queue, costs, terminate,
                    unconstrained<typename GraphT::weight_t>);
}

template <typename GraphT>
auto dijkstra_to_all(typename GraphT::node_id_t start, const GraphT &graph, MinIDQueue &queue,
                     CostVector<GraphT> &costs) {
    dijkstra_to_all(start, graph, queue, costs, [](...) { return false; });
}

template <typename GraphT>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &graph, MinIDQueue &queue, CostVector<GraphT> &costs,
              ParentVector<GraphT> &parents) {
    return dijkstra(start, target, graph, queue, costs, parents, terminate_key_min<GraphT>);
}

template <typename GraphT>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &forward_graph, const GraphT &reverse_graph, MinIDQueue &forward_queue,
              MinIDQueue &reverse_queue, CostVector<GraphT> &forward_costs,
              CostVector<GraphT> &reverse_costs, ParentVector<GraphT> &forward_parents,
              ParentVector<GraphT> &backward_parents, typename GraphT::node_id_t &middle) {
    return dijkstra(start, target, forward_graph, reverse_graph, forward_queue, reverse_queue,
                    forward_costs, reverse_costs, forward_parents, backward_parents, middle,
                    terminate_sum_min<GraphT>, no_stall<GraphT>);
}

template <typename GraphT>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &forward_graph, const GraphT &reverse_graph, MinIDQueue &forward_queue,
              MinIDQueue &reverse_queue, CostVector<GraphT> &forward_costs,
              CostVector<GraphT> &reverse_costs) {
    return dijkstra(start, target, forward_graph, reverse_graph, forward_queue, reverse_queue,
                    forward_costs, reverse_costs, terminate_sum_min<GraphT>, no_stall<GraphT>);
}

template <typename GraphT, typename TerminateFn>
auto dijkstra(typename GraphT::node_id_t start, typename GraphT::node_id_t target,
              const GraphT &forward_graph, const GraphT &reverse_graph, MinIDQueue &forward_queue,
              MinIDQueue &reverse_queue, CostVector<GraphT> &forward_costs,
              CostVector<GraphT> &reverse_costs, TerminateFn terminate) {
    return dijkstra(start, target, forward_graph, reverse_graph, forward_queue, reverse_queue,
                    forward_costs, reverse_costs, terminate, no_stall<GraphT>);
}
} // namespace common
} // namespace charge

#endif
