#ifndef CHARGE_EXPERIMENTS_CONSUMPTION_QUERIES_HPP
#define CHARGE_EXPERIMENTS_CONSUMPTION_QUERIES_HPP

#include "experiments/query.hpp"

#include "ev/charging_function_container.hpp"
#include "ev/charging_model.hpp"
#include "ev/graph.hpp"

#include "common/dijkstra.hpp"
#include "common/parallel_for.hpp"
#include "common/progress_bar.hpp"

#include <mutex>
#include <numeric>
#include <random>

namespace charge::experiments {
inline constexpr double MIN_SOC = 10; // 10 Wh minimum consumption at target

inline auto make_random_in_range_queries(const std::size_t seed,
                                         const ev::DurationGraph &duration_graph,
                                         const ev::ConsumptionGraph &min_graph,
                                         const ev::ConsumptionGraph &max_graph,
                                         const double max_capacity, const std::size_t num_queries,
                                         const std::size_t num_threads) {
    std::vector<Query> queries;

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<std::uint32_t> distribution(0, min_graph.num_nodes() - 1);

    common::MinIDQueue queue_example(min_graph.num_nodes());
    common::CostVector<ev::ConsumptionGraph> costs_example(min_graph.num_nodes(),
                                                           common::INF_WEIGHT);
    common::CostVector<ev::DurationGraph> durations_example(min_graph.num_nodes(),
                                                            common::INF_WEIGHT);

    while (queries.size() < num_queries) {
        std::vector<Query> candidates(num_queries - queries.size());
        for (auto &q : candidates) {
            q.start = distribution(generator);
            q.target = distribution(generator);
            q.min_consumption = std::numeric_limits<double>::infinity();
            q.max_consumption = std::numeric_limits<double>::infinity();
        }

        auto range = common::irange<std::size_t>(0, candidates.size());

        common::parallel_for(
            range,
            [&](const auto &range) {
                auto queue = queue_example;
                auto costs = costs_example;
                auto durations = durations_example;

                for (auto index : range) {
                    auto &q = candidates[index];

                    auto forward_min_duration = common::constrained_dijkstra(
                        q.start, q.target, duration_graph, queue, durations,
                        [&](const auto &queue) {
                            return queue.empty() || queue.peek().key > durations[q.target];
                        },
                        [](const auto cost) { return cost; });
                    auto reverse_min_duration = common::constrained_dijkstra(
                        q.target, q.start, duration_graph, queue, durations,
                        [&](const auto &queue) {
                            return queue.empty() || queue.peek().key > durations[q.start];
                        },
                        [](const auto cost) { return cost; });
                    if (forward_min_duration == common::INF_WEIGHT ||
                        reverse_min_duration == common::INF_WEIGHT)
                        continue;

                    auto min_consumption = common::constrained_dijkstra(
                        q.start, q.target, min_graph, queue, costs,
                        [&](const auto &queue) {
                            return queue.empty() ||
                                   common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                                   common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                        },
                        [=](const auto cost) {
                            return std::max<std::int32_t>(
                                std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                        });
                    auto max_consumption = common::constrained_dijkstra(
                        q.start, q.target, max_graph, queue, costs,
                        [&](const auto &queue) {
                            return queue.empty() ||
                                   common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                                   common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                        },
                        [=](const auto cost) {
                            return std::max<std::int32_t>(
                                std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                        });
                    if (min_consumption < common::INF_WEIGHT)
                        q.min_consumption = common::from_fixed(min_consumption);
                    if (max_consumption < common::INF_WEIGHT)
                        q.max_consumption = common::from_fixed(max_consumption);
                }

            },
            num_threads);

        for (auto &q : candidates) {
            if (q.min_consumption + 1 <= max_capacity) {
                q.id = queries.size();
                queries.push_back(q);
            }
        }
    }

    return queries;
}

inline auto make_random_in_charger_range_queries(
    const std::size_t seed, const ev::DurationGraph &duration_graph,
    const ev::ConsumptionGraph &min_graph, const ev::ConsumptionGraph &max_graph,
    const ev::ChargingFunctionContainer &charging_functions, const double max_capacity,
    const std::size_t num_queries, const std::size_t num_threads) {
    std::vector<Query> queries;

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<std::int32_t> distribution(0, min_graph.num_nodes() - 1);

    std::vector<std::tuple<ev::ConsumptionGraph::node_id_t, ev::ConsumptionGraph::weight_t>>
        charging_nodes;
    for (const auto node : common::irange<ev::DurationGraph::node_id_t>(0, min_graph.num_nodes())) {
        if (charging_functions.weighted(node)) {
            const auto &fn = charging_functions.weight(node);
            auto max_x = fn.function.max_x();
            auto min_y = fn.function(max_x);
            charging_nodes.push_back({node, common::to_fixed(min_y)});
        }
    }

    common::MinIDQueue queue_example(min_graph.num_nodes());
    common::CostVector<ev::ConsumptionGraph> costs_example(min_graph.num_nodes(),
                                                           common::INF_WEIGHT);
    common::CostVector<ev::DurationGraph> durations_example(min_graph.num_nodes(),
                                                            common::INF_WEIGHT);

    while (queries.size() < num_queries) {
        std::vector<Query> candidates(num_queries - queries.size());
        for (auto &q : candidates) {
            q.start = distribution(generator);
            q.target = distribution(generator);
            q.min_consumption = std::numeric_limits<double>::infinity();
            q.max_consumption = std::numeric_limits<double>::infinity();
        }

        auto range = common::irange<std::size_t>(0, candidates.size());

        common::parallel_for(
            range,
            [&](const auto &range) {
                auto queue = queue_example;
                auto costs = costs_example;
                auto durations = durations_example;

                for (auto index : range) {
                    auto &q = candidates[index];

                    auto forward_min_duration = common::constrained_dijkstra(
                        q.start, q.target, duration_graph, queue, durations,
                        [&](const auto &queue) {
                            return queue.empty() || queue.peek().key > durations[q.target];
                        },
                        [](const auto cost) { return cost; });
                    auto reverse_min_duration = common::constrained_dijkstra(
                        q.target, q.start, duration_graph, queue, durations,
                        [&](const auto &queue) {
                            return queue.empty() || queue.peek().key > durations[q.start];
                        },
                        [](const auto cost) { return cost; });
                    if (forward_min_duration == common::INF_WEIGHT ||
                        reverse_min_duration == common::INF_WEIGHT)
                        continue;

                    auto sources = charging_nodes;
                    auto iter = std::find_if(sources.begin(), sources.end(), [&](const auto &t) {
                        auto[id, val] = t;
                        return id == q.start;
                    });
                    if (iter == sources.end()) {
                        sources.push_back({q.start, 0});
                    } else {
                        std::get<1>(*iter) = 0;
                    }

                    auto min_consumption = common::constrained_dijkstra(
                        sources, q.target, min_graph, queue, costs,
                        [&](const auto &queue) {
                            return queue.empty() ||
                                   common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                                   common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                        },
                        [=](const auto cost) {
                            return std::max<std::int32_t>(
                                std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                        });
                    auto max_consumption = common::constrained_dijkstra(
                        sources, q.target, max_graph, queue, costs,
                        [&](const auto &queue) {
                            return queue.empty() ||
                                   common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                                   common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                        },
                        [=](const auto cost) {
                            return std::max<std::int32_t>(
                                std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                        });
                    if (min_consumption < common::INF_WEIGHT)
                        q.min_consumption = common::from_fixed(min_consumption);
                    if (max_consumption < common::INF_WEIGHT)
                        q.max_consumption = common::from_fixed(max_consumption);
                }

            },
            num_threads);

        for (auto &q : candidates) {
            if (q.min_consumption + 1 <= max_capacity) {
                q.id = queries.size();
                queries.push_back(q);
            }
        }
    }

    return queries;
}

inline auto make_rank_in_charger_range_queries(
    const std::size_t seed, const ev::DurationGraph &duration_graph,
    const ev::ConsumptionGraph &min_graph, const ev::ConsumptionGraph &max_graph,
    const ev::ChargingFunctionContainer &charging_functions, const double max_capacity,
    const std::size_t num_queries, const std::size_t num_threads) {
    std::vector<Query> queries;

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<std::int32_t> distribution(0, min_graph.num_nodes() - 1);

    std::vector<std::tuple<ev::ConsumptionGraph::node_id_t, ev::ConsumptionGraph::weight_t>>
        charging_nodes;
    for (const auto node : common::irange<ev::DurationGraph::node_id_t>(0, min_graph.num_nodes())) {
        if (charging_functions.weighted(node)) {
            const auto &fn = charging_functions.weight(node);
            auto max_x = fn.function.max_x();
            auto min_y = fn.function(max_x);
            charging_nodes.push_back({node, common::to_fixed(min_y)});
        }
    }

    common::MinIDQueue queue_example(min_graph.num_nodes());
    common::CostVector<ev::ConsumptionGraph> costs_example(min_graph.num_nodes(),
                                                           common::INF_WEIGHT);
    common::CostVector<ev::DurationGraph> durations_example(min_graph.num_nodes(),
                                                            common::INF_WEIGHT);

    std::vector<std::int32_t> sources(num_queries);
    for (auto &s : sources) {
        s = distribution(generator);
    }

    std::vector<Query> candidates;
    auto sources_range = common::irange<std::size_t>(0, sources.size());
    common::parallel_for(
        sources_range,
        [&](const auto &range) {
            auto queue = queue_example;
            auto durations = durations_example;
            std::vector<std::int32_t> targets(min_graph.num_nodes());
            std::iota(targets.begin(), targets.end(), 0);

            for (auto index : range) {
                auto source = sources[index];

                common::dijkstra_to_all(source, duration_graph, queue, durations);
                std::sort(targets.begin(), targets.end(), [&](const auto lhs, const auto rhs) {
                    return durations[lhs] < durations[rhs];
                });

                unsigned rank = 0;
                while (1 << rank < targets.size()) {
                    candidates.push_back(
                        {source, targets[1 << rank], std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity(), common::INVALID_ID, rank});
                    rank++;
                }
            }
        },
        num_threads);

    auto candidates_range = common::irange<std::size_t>(0, candidates.size());
    common::parallel_for(
        candidates_range,
        [&](const auto &range) {
            auto queue = queue_example;
            auto costs = costs_example;
            auto durations = durations_example;

            for (auto index : range) {
                auto &q = candidates[index];

                auto reverse_min_duration = common::constrained_dijkstra(
                    q.target, q.start, duration_graph, queue, durations,
                    [&](const auto &queue) {
                        return queue.empty() || queue.peek().key > durations[q.start];
                    },
                    [](const auto cost) { return cost; });
                // Fix for s and t not being the same SCC
                if (reverse_min_duration == common::INF_WEIGHT)
                    continue;

                auto sources = charging_nodes;
                auto iter = std::find_if(sources.begin(), sources.end(), [&](const auto &t) {
                    auto[id, val] = t;
                    return id == q.start;
                });
                if (iter == sources.end()) {
                    sources.push_back({q.start, 0});
                } else {
                    std::get<1>(*iter) = 0;
                }

                auto min_consumption = common::constrained_dijkstra(
                    sources, q.target, min_graph, queue, costs,
                    [&](const auto &queue) {
                        return queue.empty() ||
                               common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                               common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                    },
                    [=](const auto cost) {
                        return std::max<std::int32_t>(
                            std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                    });
                auto max_consumption = common::constrained_dijkstra(
                    sources, q.target, max_graph, queue, costs,
                    [&](const auto &queue) {
                        return queue.empty() ||
                               common::from_fixed(costs[q.target]) + MIN_SOC < max_capacity ||
                               common::from_fixed(queue.peek().key) + MIN_SOC > max_capacity;
                    },
                    [=](const auto cost) {
                        return std::max<std::int32_t>(
                            std::min<std::int32_t>(cost, common::to_fixed(max_capacity)), 0);
                    });
                if (min_consumption < common::INF_WEIGHT)
                    q.min_consumption = common::from_fixed(min_consumption);
                if (max_consumption < common::INF_WEIGHT)
                    q.max_consumption = common::from_fixed(max_consumption);
            }

        },
        num_threads);

    for (auto &q : candidates) {
        if (q.min_consumption + 1 <= max_capacity) {
            queries.push_back(q);
        }
    }

    std::stable_sort(queries.begin(), queries.end(),
                     [](const auto &lhs, const auto &rhs) { return lhs.rank < rhs.rank; });
    for (auto id : common::irange<unsigned>(0, queries.size())) {
        queries[id].id = id;
    }

    return queries;
}
} // namespace charge::experiments

#endif
