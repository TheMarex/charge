#ifndef CHARGE_PREPROCESSING_NODE_PRIORITY_HPP
#define CHARGE_PREPROCESSING_NODE_PRIORITY_HPP

#include "common/dijkstra.hpp"
#include "common/id_queue.h"

namespace charge::preprocessing
{

template<typename GraphT>
unsigned compute_priority(const typename GraphT::node_id_t via,
                  const std::vector<unsigned> &levels,
                  const std::vector<bool> &contracted,
                  const GraphT &forward_graph, const GraphT &reverse_graph,
                  common::MinIDQueue &forward_queue, common::MinIDQueue &reverse_queue,
                  common::CostVector &forward_costs, common::CostVector &reverse_costs)
{
    std::size_t to_insert = 0;
    std::size_t to_delete = 0;
    std::size_t inserted_hops = 0;
    std::size_t deleted_hops = 0;

    for (auto edge_in = reverse_graph.begin(via); edge_in < reverse_graph.end(via); ++edge_in)
    {
        auto start = reverse_graph.target(edge_in);
        if (start == via)
            continue;

        if (contracted[start])
            continue;

        to_delete++;
        deleted_hops += reverse_graph.data(edge_in).hops;

        for (auto edge_out = forward_graph.begin(via); edge_out < forward_graph.end(via); ++edge_out)
        {
            auto target = forward_graph.target(edge_out);
            if (target == via)
                continue;

            if (start == target)
                continue;

            if (contracted[target])
                continue;

            auto weight = reverse_graph.weight(edge_in) + forward_graph.weight(edge_out);

            const constexpr std::size_t MAX_ITERATIONS = 1000;
            std::size_t iterations = 0;
            auto const vitness_termination = [weight, &iterations](auto &forward_queue, auto &reverse_queue, const unsigned) {
                return (iterations++ > MAX_ITERATIONS) ||
                       (!forward_queue.empty() && !reverse_queue.empty() &&
                        forward_queue.peek().key + reverse_queue.peek().key > weight);
            };
            auto const stall_at_lower = [&contracted, via](const unsigned key) {
                return key == via || contracted[key];
            };

            const auto best_cost = dijkstra(start, target, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs, vitness_termination, stall_at_lower);

            // found a vitness that this is not the best path
            if (weight >= best_cost)
            {
                continue;
            }
            to_insert++;
            inserted_hops += reverse_graph.data(edge_in).hops + forward_graph.data(edge_out).hops;
        }
    }

    if (deleted_hops > 0 && to_delete > 0)
    {
        return 1000.0 * levels[via] + (2000.0 * to_insert)/to_delete + (4000.0 * inserted_hops)/deleted_hops;
    }

    return 1000.0 * levels[via];
}

}

#endif
