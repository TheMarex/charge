#ifndef CHARGE_PREPROCESSING_CONTRACTOR_HPP
#define CHARGE_PREPROCESSING_CONTRACTOR_HPP

#include "common/graph_transform.hpp"
#include "common/dyn_graph.hpp"
#include "common/dijkstra.hpp"
#include "common/progress_bar.hpp"

#include "preprocessing/node_priority.hpp"

namespace charge::preprocessing
{

struct ContractData
{
    ContractData() : hops(1) {}
    unsigned hops;
};
typedef DynDataGraph<ContractData> ContractGraph;

namespace detail
{
    // for graphs without data we don't need to keep stats
    template<typename GraphT>
    void update_data(const typename GraphT::edge_id_t shortcut_fwd,
                     const typename GraphT::edge_id_t shortcut_rev,
                     const typename GraphT::data_t edge_data_in,
                     const typename GraphT::data_t edge_data_out,
                     GraphT& forward_graph, GraphT& reverse_graph)
    {
    }

    // for graphs with contraction data we need to keep track of some stats
    template<>
    void update_data<ContractGraph>(const typename ContractGraph::edge_id_t shortcut_fwd,
                                    const typename ContractGraph::edge_id_t shortcut_rev,
                                    const typename ContractGraph::data_t edge_data_in,
                                    const typename ContractGraph::data_t edge_data_out,
                                    ContractGraph& forward_graph, ContractGraph& reverse_graph)
    {
        forward_graph.data(shortcut_fwd).hops = edge_data_in.hops + edge_data_out.hops;
        reverse_graph.data(shortcut_rev).hops = edge_data_in.hops + edge_data_out.hops;
    }

    template<typename GraphT>
    struct InsertionBuffer
    {
        typename GraphT::edge_t edge_fwd;
        typename GraphT::edge_t edge_rev;
        typename GraphT::data_t data_in;
        typename GraphT::data_t data_out;
    };

    template<typename GraphT>
    unsigned contract_node(const typename GraphT::node_id_t via,
                      const std::vector<bool> &contracted,
                      std::vector<InsertionBuffer<GraphT>> &insert_buffer,
                      GraphT &forward_graph, GraphT &reverse_graph,
                      MinIDQueue &forward_queue, MinIDQueue &reverse_queue,
                      CostVector &forward_costs, CostVector &reverse_costs)
    {
        for (auto edge_in = reverse_graph.begin(via); edge_in < reverse_graph.end(via); ++edge_in)
        {
            auto start = reverse_graph.target(edge_in);
            if (start == via)
                continue;

            if (contracted[start])
                continue;

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

                const constexpr std::size_t MAX_ITERATIONS = 3000;
                std::size_t iterations = 0;
                auto const vitness_termination = [weight, &iterations](MinIDQueue &forward_queue, MinIDQueue &reverse_queue, const unsigned) {
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

                auto maybe_edge = forward_graph.edge(start, target);
                if (maybe_edge == invalid_id)
                {
                    insert_buffer.emplace_back(InsertionBuffer<GraphT>{typename GraphT::edge_t {start, target, weight},
                                               typename GraphT::edge_t {target, start, weight},
                                               reverse_graph.data(edge_in),
                                               forward_graph.data(edge_out)});
                }
                else
                {
                    assert(forward_graph.weight(maybe_edge) > weight);

                    forward_graph.weight(maybe_edge) = weight;
                    auto reverse_edge = reverse_graph.edge(target, start);
                    assert(reverse_edge != invalid_id);
                    reverse_graph.weight(reverse_edge) = weight;

                    update_data(maybe_edge, reverse_edge, reverse_graph.data(edge_in), forward_graph.data(edge_out), forward_graph, reverse_graph);
                }
            }
        }
    }

    template<typename GraphT>
    void update_levels(typename GraphT::node_id_t node,
                       const std::vector<bool> &contracted,
                       const GraphT &forward_graph,
                       const GraphT &reverse_graph,
                       std::vector<unsigned> &levels,
                       std::vector<typename GraphT::node_id_t> &to_update)
    {
        // get all neighbors whose priority could have changed
        for (auto edge_out = forward_graph.begin(node); edge_out < forward_graph.end(node); ++edge_out)
        {
            auto target = forward_graph.target(edge_out);
            if (!contracted[target])
            {
                levels[target] = std::max(levels[node]+1, levels[target]);
                to_update.push_back(target);
            }
        }
        for (auto edge_in = reverse_graph.begin(node); edge_in < reverse_graph.end(node); ++edge_in)
        {
            auto target = reverse_graph.target(edge_in);
            if (!contracted[target])
            {
                levels[target] = std::max(levels[node]+1, levels[target]);
                to_update.push_back(target);
            }
        }
    }

    template<typename GraphT>
    void update_priorities(const std::vector<typename GraphT::node_id_t> &to_update,
                           const std::vector<unsigned> &levels,
                           const std::vector<bool> &contracted,
                           GraphT &forward_graph, GraphT &reverse_graph,
                           MinIDQueue &forward_queue, MinIDQueue &reverse_queue,
                           CostVector &forward_costs, CostVector &reverse_costs,
                           MinIDQueue &queue)
    {
        for (auto node : to_update)
        {
            if (contracted[node]) continue;

            auto priority = compute_priority(node, levels, contracted, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs);
            const auto key = queue.get_key(node);
            if (key > priority)
            {
                queue.decrease_key(IDKeyPair {node, priority});
            }
            else if (key < priority)
            {
                queue.increase_key(IDKeyPair {node, priority});
            }
        }
    }
}

void contract(DynGraph& forward_graph, DynGraph &reverse_graph, const std::vector<DynGraph::node_id_t>& order, const std::vector<unsigned>& rank)
{
    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    CostVector forward_costs(forward_graph.num_nodes(), inf_weight);
    CostVector reverse_costs(reverse_graph.num_nodes(), inf_weight);

    std::vector<bool> contracted(forward_graph.num_nodes(), false);

    ProgressBar progress(order.size());

    auto node_idx = 0u;
    std::vector<detail::InsertionBuffer<DynGraph>> insert_buffer;
    for (auto via : order)
    {
        progress.update(node_idx++);

        detail::contract_node(via, contracted, insert_buffer, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs);

        for (const auto& entry : insert_buffer)
        {
            auto shortcut_fwd = forward_graph.insert(entry.edge_fwd);
            auto shortcut_rev = reverse_graph.insert(entry.edge_rev);
            detail::update_data(shortcut_fwd, shortcut_rev, entry.data_in, entry.data_out, forward_graph, reverse_graph);
        }
        insert_buffer.clear();

        contracted[via] = true;
    }
}

#define MIN_UPDATE_SIZE 0

std::vector<unsigned> contract(ContractGraph& forward_graph, ContractGraph &reverse_graph)
{
    typedef typename DynGraph::node_id_t node_id_t;

    std::vector<node_id_t> order;
    order.reserve(forward_graph.num_nodes());

    MinIDQueue queue(forward_graph.num_nodes());
    std::vector<bool> contracted(forward_graph.num_nodes(), false);
    std::vector<unsigned> levels(forward_graph.num_nodes(), 0);

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    CostVector forward_costs(forward_graph.num_nodes(), inf_weight);
    CostVector reverse_costs(reverse_graph.num_nodes(), inf_weight);

    ProgressBar progress_pq(forward_graph.num_nodes());
    for (node_id_t node = 0; node < forward_graph.num_nodes(); ++node)
    {
        progress_pq.update(node);
        auto priority = compute_priority(node, levels, contracted, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs);
        queue.push(IDKeyPair {node, priority});
    }

    ProgressBar progress(forward_graph.num_nodes());

    auto node_idx = 0u;
    auto inserted = 0;
    auto updated = 0;
    std::vector<node_id_t> to_update;
    std::vector<detail::InsertionBuffer<ContractGraph>> insert_buffer;
    while (!queue.empty())
    {
        progress.update(node_idx++);

        auto top = queue.pop();
        order.emplace_back(top.id);
        contracted[top.id] = true;

        detail::contract_node(top.id, contracted, insert_buffer, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs);

        for (const auto& entry : insert_buffer)
        {
            auto shortcut_fwd = forward_graph.insert(entry.edge_fwd);
            auto shortcut_rev = reverse_graph.insert(entry.edge_rev);
            detail::update_data(shortcut_fwd, shortcut_rev, entry.data_in, entry.data_out, forward_graph, reverse_graph);
        }
        insert_buffer.clear();

        detail::update_levels(top.id, contracted, forward_graph, reverse_graph, levels, to_update);

        if (to_update.size() > MIN_UPDATE_SIZE)
        {
            std::sort(to_update.begin(), to_update.end());
            auto new_to_update_end = std::unique(to_update.begin(), to_update.end());;
            to_update.resize(new_to_update_end - to_update.begin());

            detail::update_priorities(to_update, levels, contracted, forward_graph, reverse_graph, forward_queue, reverse_queue, forward_costs, reverse_costs, queue);
            to_update.clear();
        }
    }

    return order;
}

}

#endif
