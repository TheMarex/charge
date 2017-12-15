#ifndef GRAPH_UTILS_HPP
#define GRAPH_UTILS_HPP

#include "common/constants.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace charge {
namespace common {

template <typename T, typename = int> struct is_weighted_edge : std::false_type {};

template <typename T> struct is_weighted_edge<T, decltype((void)T::weight, 0)> : std::true_type {};

template <typename EdgeT> void invertEdges(std::vector<EdgeT> &edges) {
    if constexpr(is_weighted_edge<EdgeT>::value)
    {
        std::transform(edges.begin(), edges.end(), edges.begin(), [](const EdgeT &edge) {
            return EdgeT{edge.target, edge.start, edge.weight};
        });
    }
    else
    {
        std::transform(edges.begin(), edges.end(), edges.begin(), [](const EdgeT &edge) {
            return EdgeT{edge.target, edge.start};
        });
    }
}

template <typename EdgeT> void deduplicateEdges(std::vector<EdgeT> &edges) {
    auto new_end = std::unique(edges.begin(), edges.end(), [](const EdgeT &lhs, const EdgeT &rhs) {
        return lhs.start == rhs.start && lhs.target == rhs.target;
    });
    edges.resize(new_end - edges.begin());
}

// Returns a map old -> new
template <typename EdgeT>
std::pair<std::size_t, std::unordered_map<typename EdgeT::node_t, typename EdgeT::node_t>>
renumberEdges(std::vector<EdgeT> &sorted_edges) {
    std::vector<typename EdgeT::node_t> node_ids;
    for (const auto &e : sorted_edges) {
        node_ids.push_back(e.start);
        node_ids.push_back(e.target);
    }
    std::sort(node_ids.begin(), node_ids.end());
    auto new_end = std::unique(node_ids.begin(), node_ids.end());
    node_ids.resize(new_end - node_ids.begin());

    std::unordered_map<typename EdgeT::node_t, typename EdgeT::node_t> old_to_new;
    for (typename EdgeT::node_t new_node_id = 0; new_node_id < node_ids.size(); ++new_node_id) {
        old_to_new[node_ids[new_node_id]] = new_node_id;
    }
    for (auto &e : sorted_edges) {
        e.start = old_to_new[e.start];
        e.target = old_to_new[e.target];
    }

    return std::make_pair(node_ids.size(), std::move(old_to_new));
}

template <typename EdgeT>
void renumberEdges(std::vector<EdgeT> &edges, std::vector<typename EdgeT::node_t> &permutation) {
    for (auto &e : edges) {
        e.start = permutation[e.start];
        e.target = permutation[e.target];
    }
}

template <typename GraphT> GraphT toUndirected(const GraphT &graph) {
    auto edges = graph.edges();
    auto reverse_edges = edges;
    invertEdges(reverse_edges);
    edges.insert(edges.end(), reverse_edges.begin(), reverse_edges.end());
    reverse_edges.clear();

    std::sort(edges.begin(), edges.end());
    using edge_t = typename GraphT::edge_t;
    auto new_end =
        std::unique(edges.begin(), edges.end(), [](const edge_t &lhs, const edge_t &rhs) {
            return lhs.start == rhs.start && lhs.target == rhs.target;
        });
    edges.resize(new_end - edges.begin());
    return GraphT{graph.num_nodes(), std::move(edges)};
}

template <typename GraphT> GraphT invert(const GraphT &graph) {
    auto edges = graph.edges();
    invertEdges(edges);

    std::sort(edges.begin(), edges.end());
    return GraphT{graph.num_nodes(), std::move(edges)};
}

template <typename NodeT> std::vector<unsigned> orderToRank(const std::vector<NodeT> &order) {
    std::vector<unsigned> rank;
    rank.reserve(order.size());
    for (unsigned r = 0; r < order.size(); ++r) {
        rank.emplace_back(r);
    }
    std::sort(rank.begin(), rank.end(),
              [&order](const unsigned lhs, const unsigned rhs) { return order[lhs] < order[rhs]; });

    return rank;
}

template <typename GraphT>
GraphT filterByRank(const GraphT &graph, const std::vector<unsigned> &rank) {
    std::vector<typename GraphT::edge_t> remaining_edges;

    for (auto node = 0; node < graph.num_nodes(); ++node) {
        for (auto edge = graph.begin(node); edge < graph.end(node); ++edge) {
            auto target = graph.target(edge);
            auto weight = graph.weight(edge);
            if (rank[node] <= rank[target]) {
                remaining_edges.emplace_back(node, target, weight);
            }
        }
    }

    return GraphT{graph.num_nodes(), std::move(remaining_edges)};
}

template <typename GraphT> auto computeDegree(const GraphT &graph) {
    std::vector<std::size_t> degree(graph.num_nodes(), 0);
    auto undirected_graph = common::toUndirected(graph);

    for (auto node : undirected_graph.nodes()) {
        degree[node] = undirected_graph.end(node) - undirected_graph.begin(node);
    }

    return degree;
}

template <typename GraphT> auto computeComponents(const GraphT &undirected_graph) {
    std::vector<std::size_t> component(undirected_graph.num_nodes(), common::INVALID_ID);
    std::vector<std::size_t> component_sizes;
    std::size_t component_id = 0;
    std::size_t component_size = 0;
    for (auto node : undirected_graph.nodes()) {
        if (component[node] != common::INVALID_ID)
            continue;

        std::vector<std::uint32_t> nodes;
        nodes.push_back(node);

        component[node] = component_id;
        component_size = 1;

        while (!nodes.empty()) {
            auto start = nodes.back();
            nodes.pop_back();

            for (auto edge : undirected_graph.edges(start)) {
                auto target = undirected_graph.target(edge);
                if (component[target] != common::INVALID_ID)
                    continue;

                component[target] = component_id;
                component_size++;
                nodes.push_back(target);
            }
        }

        component_sizes.push_back(component_size);
        component_id++;
    }

    return std::make_tuple(std::move(component), std::move(component_sizes));
}

template <typename GraphT, typename TurnCostModelT>
GraphT toTurnGraph(const GraphT &graph, const TurnCostModelT &turn_cost_model) {
    auto degree = computeDegree(toUndirected(graph));
    std::vector<typename GraphT::edge_t> turn_edges;

    for (auto from : graph.nodes()) {
        for (auto from_edge : graph.edges(from)) {
            auto via = graph.target(from_edge);
            for (auto to_edge : graph.edges(via)) {
                auto to = graph.target(to_edge);

                if (from == to && degree[via] == 2)
                    continue;

                turn_edges.push_back(typename GraphT::edge_t{
                    from_edge, to_edge, turn_cost_model(graph, degree[via], from, via, to)});
            }
        }
    }

    return GraphT{graph.num_edges(), std::move(turn_edges)};
}

template <typename GraphT> auto edgeToStartNode(const GraphT &graph) {
    std::vector<typename GraphT::edge_id_t> edge_to_start_node(graph.num_edges());

    for (auto from = 0; from < graph.num_nodes(); ++from) {
        for (auto from_edge = graph.begin(from); from_edge < graph.end(from); ++from_edge) {
            edge_to_start_node[from_edge] = from;
        }
    }

    return edge_to_start_node;
}

} // namespace common
} // namespace charge

#endif
