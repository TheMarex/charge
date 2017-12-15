#ifndef CHARGE_COMMON_NODE_POTENTIALS_HPP
#define CHARGE_COMMON_NODE_POTENTIALS_HPP

#include "common/dijkstra.hpp"

#include <cstdint>

namespace charge::common {

template <typename GraphT> class ZeroNodePotentials {
  public:
    using node_id_t = typename GraphT::node_id_t;
    using key_t = std::int32_t;

    template <typename CostT>
    inline key_t key(const node_id_t, const key_t default_key, const CostT &) const {
        return default_key;
    }

    template <typename CostT>
    inline bool check_consitency(const node_id_t, const node_id_t, const CostT &) const {
        return true;
    }
};

template <typename GraphT> class LandmarkNodePotentials {
  public:
    using node_id_t = typename GraphT::node_id_t;
    using key_t = std::int32_t;

    LandmarkNodePotentials(const GraphT &reverse_graph)
        : reverse_graph(reverse_graph), cost_to_target(reverse_graph.num_nodes(), INF_WEIGHT) {}

    template <typename CostT>
    inline key_t key(const node_id_t node, const key_t default_key, const CostT &) const {
        if (cost_to_target[node] == common::INF_WEIGHT)
            return common::INF_WEIGHT;

        return default_key + cost_to_target[node];
    }

    template <typename CostT>
    inline bool check_consitency(const node_id_t u, const node_id_t v, const CostT &f_uv) const {
        return to_upper_fixed(f_uv.min_x) - cost_to_target[u] + cost_to_target[v] >= 0;
    }

    void recompute(MinIDQueue& queue, const node_id_t target) {
        dijkstra_to_all(target, reverse_graph, queue, cost_to_target);
    }

  private:
    const GraphT &reverse_graph;
    CostVector<GraphT> cost_to_target;
};

template <typename GraphT> class LazyLandmarkNodePotentials {
  public:
    using node_id_t = typename GraphT::node_id_t;
    using key_t = std::int32_t;

    LazyLandmarkNodePotentials(const GraphT &reverse_graph)
        : reverse_graph(reverse_graph), queue(reverse_graph.num_nodes()), cost_to_target(reverse_graph.num_nodes(), INF_WEIGHT), settled(reverse_graph.num_nodes(), false) {}

    template <typename CostT>
    inline key_t key(const node_id_t node, const key_t default_key, const CostT &) const {
        if (!settled[node])
        {
            continue_dijkstra(node, reverse_graph, queue, cost_to_target, settled);
        }
        if (cost_to_target[node] == common::INF_WEIGHT)
            return common::INF_WEIGHT;

        return default_key + cost_to_target[node];
    }

    template <typename CostT>
    inline bool check_consitency(const node_id_t u, const node_id_t v, const CostT &f_uv) const {
        return to_upper_fixed(f_uv.min_x) - cost_to_target[u] + cost_to_target[v] >= 0;
    }

    void recompute(const node_id_t source, const node_id_t target) {
        dijkstra(target, source, reverse_graph, queue, cost_to_target, settled);
    }

  private:
    const GraphT &reverse_graph;
    mutable MinIDQueue queue;
    mutable std::vector<bool> settled;
    mutable CostVector<GraphT> cost_to_target;
};
}

#endif
