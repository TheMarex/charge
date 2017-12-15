#ifndef CHARGE_COMMON_DYN_GRAPH_HPP
#define CHARGE_COMMON_DYN_GRAPH_HPP

#include "common/edge.hpp"
#include "common/irange.hpp"

#include "common/constants.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

#include <iostream>

#define OVER_ALLOCATION 1.1
#define MAX_OVER_ALLOCATION 1.3

namespace charge::common {

template <typename WeightT> class DynDataGraph {
  public:
    using node_id_t = std::uint32_t;
    using edge_id_t = std::uint32_t;
    using weight_t = WeightT;
    using edge_t = Edge<node_id_t, weight_t>;
    using node_range_t = decltype(irange<node_id_t>(0, 1));
    using edge_range_t = decltype(irange<edge_id_t>(0, 1));

    DynDataGraph(std::size_t num_nodes_, const std::vector<edge_t> &sorted_edges) {
        fromEdges(num_nodes_, sorted_edges);
    }

    std::size_t num_nodes() const { return begin_edges.size(); }

    std::size_t num_edges() const { return num_edges_; }

    edge_id_t begin(node_id_t node) const { return begin_edges[node]; }

    edge_id_t end(node_id_t node) const { return end_edges[node]; }

    node_id_t target(edge_id_t edge) const { return targets[edge]; }

    const weight_t &weight(edge_id_t edge) const { return weights[edge]; }

    weight_t &weight(edge_id_t edge) { return weights[edge]; }

    edge_id_t edge(node_id_t start_node, node_id_t target_node) const {
        for (auto edge = begin(start_node); edge < end(start_node); ++edge) {
            if (target(edge) == target_node) {
                return edge;
            }
        }

        return INVALID_ID;
    }

    node_range_t nodes() const { return irange<node_id_t>(0, num_nodes()); }

    edge_range_t edges(node_id_t node) const { return irange<edge_id_t>(begin(node), end(node)); }

    std::vector<edge_t> edges() const {
        std::vector<edge_t> edges;
        edges.reserve(num_edges());

        for (node_id_t node = 0; node < num_nodes(); ++node) {
            for (auto edge = begin(node); edge < end(node); ++edge) {
                edges.emplace_back(node, target(edge), weight(edge));
            }
        }

        return edges;
    }

    void remove(const node_id_t node, const edge_id_t edge) {
        assert(end(node) > begin(node));
        std::swap(targets[edge], targets[end(node) - 1]);
        std::swap(weights[edge], weights[end(node) - 1]);
        mark_free(end(node) - 1);
        end_edges[node]--;
    }

    void remove(const edge_t &e) {
        for (auto candidate = begin(e.start); candidate < end(e.start); ++candidate) {
            if (target(candidate) == e.target) {
                remove(e.start, candidate);
            }
        }
    }

    edge_id_t insert(const edge_t &e) {
        const auto old_begin = begin(e.start);
        const auto old_end = end(e.start);
        const auto old_num_edges = old_end - old_begin;

        edge_id_t inserted_id = INVALID_ID;

        // first edge of the block
        if (old_num_edges == 0) {
            auto old_size = targets.size();
            // insert new edge
            targets.push_back(e.target);
            weights.push_back(e.weight);
            begin_edges[e.start] = old_size;
            end_edges[e.start] = targets.size();
            inserted_id = old_size;

            assert(targets[end_edges[e.start] - 1] == e.target);
        }
        // check if there is free space after the block
        else if (old_end < targets.size() && is_free(old_end)) {
            end_edges[e.start]++;
            targets[old_end] = e.target;
            weights[old_end] = e.weight;
            inserted_id = old_end;
        }
        // check if there is free space before the block
        else if (old_begin > 0 && is_free(old_begin - 1)) {
            begin_edges[e.start]--;
            targets[old_begin - 1] = e.target;
            weights[old_begin - 1] = e.weight;
            inserted_id = old_begin - 1;
        }
        // relocate block at the end of the array
        else {
            const auto old_size = targets.size();
            const auto new_size = old_size + (old_num_edges + 1) * OVER_ALLOCATION;
            // make sure we don't see a reallocation during insertion
            // if we don't do this, we the iterators will get invalidated
            targets.reserve(new_size);
            weights.reserve(new_size);

            // relocate block
            std::copy(targets.begin() + old_begin, targets.begin() + old_end,
                      std::back_inserter(targets));
            std::copy(weights.begin() + old_begin, weights.begin() + old_end,
                      std::back_inserter(weights));
            // invalidate old block
            std::fill(targets.begin() + old_begin, targets.begin() + old_end, INVALID_ID);
            // insert new edge
            targets.push_back(e.target);
            weights.push_back(e.weight);
            inserted_id = targets.size() - 1;

            begin_edges[e.start] = old_size;
            end_edges[e.start] = targets.size();

            // create some free elemts after the block
            while (targets.size() < new_size) {
                targets.push_back(INVALID_ID);
                weights.push_back({});
            }

            assert(targets[end_edges[e.start] - 1] == e.target);
        }
        num_edges_++;

        if (is_degenerated()) {
            shrink_to_fit();
            // the edge iterator got invalidated by the above cleanup
            inserted_id = edge(e.start, e.target);
        }

        assert(inserted_id != INVALID_ID);
        return inserted_id;
    }

    void insert(const std::vector<edge_t> &to_insert) {
        for (const auto &edge : to_insert) {
            insert(edge);
        }
    }

    void remove(const std::vector<edge_t> &to_remove) {
        for (const auto &edge : to_remove) {
            remove(edge);
        }
    }

    void shrink_to_fit() {
        auto input_edges = edges();
        std::sort(input_edges.begin(), input_edges.end());

        fromEdges(num_nodes(), input_edges);
    }

  private:
    void fromEdges(std::size_t num_nodes_, const std::vector<edge_t> &sorted_edges) {
        begin_edges.clear();
        end_edges.clear();
        targets.clear();
        weights.clear();

        begin_edges.reserve(num_nodes_);
        end_edges.reserve(num_nodes_);
        targets.reserve(sorted_edges.size());
        weights.reserve(sorted_edges.size());

        assert(std::is_sorted(
            sorted_edges.begin(), sorted_edges.end(), [](const auto &lhs, const auto &rhs) {
                return std::tie(lhs.start, lhs.target) < std::tie(rhs.start, rhs.target);
            }));
        auto last_id = INVALID_ID;
        for (const auto &edge : sorted_edges) {
            // we need this to fill any gaps
            while (begin_edges.size() < edge.start) {
                begin_edges.push_back(targets.size());
            }

            if (last_id != edge.start) {
                begin_edges.push_back(targets.size());
                last_id = edge.start;
            }
            targets.push_back(edge.target);
            weights.push_back(edge.weight);
        }
        // fill up graps at the end
        while (begin_edges.size() < num_nodes_) {
            begin_edges.push_back(targets.size());
        }
        // save the end of the block as well
        for (unsigned node = 0; node < num_nodes_ - 1; ++node) {
            end_edges.push_back(begin_edges[node + 1]);
        }
        end_edges.push_back(targets.size());

        num_edges_ = sorted_edges.size();
        assert(begin_edges.size() == num_nodes_);
        assert(end_edges.size() == num_nodes_);
        assert(targets.size() == sorted_edges.size());
        assert(weights.size() == sorted_edges.size());
    }

    bool is_free(const edge_id_t edge) const { return targets[edge] == INVALID_ID; }

    bool is_degenerated() const {
        const auto over_allocation = targets.size() / (double)num_edges_;
        return over_allocation > MAX_OVER_ALLOCATION;
    }

    void mark_free(const edge_id_t edge) { targets[edge] = INVALID_ID; }

    std::size_t num_edges_;
    std::vector<edge_id_t> begin_edges;
    std::vector<edge_id_t> end_edges;
    std::vector<node_id_t> targets;
    std::vector<weight_t> weights;
};

struct empty_data_t {};

typedef DynDataGraph<empty_data_t> DynGraph;
}

#endif
