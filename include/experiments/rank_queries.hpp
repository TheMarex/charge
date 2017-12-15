#ifndef CHARGE_EXPERIMENTS_RANK_QUERIES_HPP
#define CHARGE_EXPERIMENTS_RANK_QUERIES_HPP

#include "experiments/query.hpp"

#include "ev/graph.hpp"

#include "common/dijkstra.hpp"
#include "common/parallel_for.hpp"
#include "common/progress_bar.hpp"

#include <mutex>
#include <numeric>
#include <random>

namespace charge::experiments {

inline auto make_rank(const ev::DurationGraph &graph, std::vector<Query> queries,
                      const std::size_t num_threads) {

    common::MinIDQueue queue_example(graph.num_nodes());
    common::CostVector<ev::ConsumptionGraph> costs_example(graph.num_nodes(), common::INF_WEIGHT);

    auto range = common::irange<std::size_t>(0, queries.size());

    common::parallel_for(
        range,
        [&](const auto &range) {
            auto queue = queue_example;
            auto costs = costs_example;
            std::vector<ev::DurationGraph::node_id_t> nodes(graph.num_nodes());
            std::iota(nodes.begin(), nodes.end(), 0);

            for (auto index : range) {
                auto &q = queries[index];

                common::dijkstra_to_all(q.start, graph, queue, costs);
                std::sort(nodes.begin(), nodes.end(),
                          [&](const auto lhs, const auto rhs) { return costs[lhs] < costs[rhs]; });
                auto iter = std::find(nodes.begin(), nodes.end(), q.target);
                std::uint32_t rank = std::log2(std::distance(nodes.begin(), iter) + 1);

                q.rank = rank;
            }

        },
        num_threads);

    return queries;
}

} // namespace charge::experiments

#endif
