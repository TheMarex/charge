#ifndef CHARGE_COMMON_EXPERIMENT_FACTORY_HPP
#define CHARGE_COMMON_EXPERIMENT_FACTORY_HPP

#include "experiments/consumption_queries.hpp"
#include "experiments/random_queries.hpp"

namespace charge::experiments {

namespace detail {
inline auto make_random_experiment(std::size_t seed, std::size_t num_queries,
                                   std::size_t num_nodes) {
    return make_random_queries(seed, num_nodes, num_queries);
}

inline auto make_random_in_range_experiment(std::size_t seed, std::size_t num_queries,
                                            const ev::DurationGraph &min_duration_graph,
                                            const ev::ConsumptionGraph &min_consumption_graph,
                                            const ev::ConsumptionGraph &max_consumption_graph,
                                            const double max_capacity, std::size_t num_threads) {
    return make_random_in_range_queries(seed, min_duration_graph, min_consumption_graph,
                                        max_consumption_graph, max_capacity, num_queries,
                                        num_threads);
}

inline auto
make_random_in_charger_range_experiment(std::size_t seed, std::size_t num_queries,
                                        const ev::DurationGraph &min_duration_graph,
                                        const ev::ConsumptionGraph &min_consumption_graph,
                                        const ev::ConsumptionGraph &max_consumption_graph,
                                        const ev::ChargingFunctionContainer &chargers,
                                        const double max_capacity, std::size_t num_threads) {
    return make_random_in_charger_range_queries(seed, min_duration_graph, min_consumption_graph,
                                                max_consumption_graph, chargers, max_capacity,
                                                num_queries, num_threads);
}

inline auto make_rank_in_charger_range_experiment(std::size_t seed, std::size_t num_queries,
                                                  const ev::DurationGraph &min_duration_graph,
                                                  const ev::ConsumptionGraph &min_consumption_graph,
                                                  const ev::ConsumptionGraph &max_consumption_graph,
                                                  const ev::ChargingFunctionContainer &chargers,
                                                  const double max_capacity,
                                                  std::size_t num_threads) {
    return make_rank_in_charger_range_queries(seed, min_duration_graph, min_consumption_graph,
                                                max_consumption_graph, chargers, max_capacity,
                                                num_queries, num_threads);
}
} // namespace detail

auto make_experiment(const std::string &name, std::size_t seed, std::size_t num_queries,
                     const ev::DurationGraph &min_duration_graph,
                     const ev::ConsumptionGraph &min_consumption_graph,
                     const ev::ConsumptionGraph &max_consumption_graph,
                     const ev::ChargingFunctionContainer &chargers, const double max_capacity,
                     std::size_t num_threads) {
    if (name == "random")
        return detail::make_random_experiment(seed, num_queries, min_consumption_graph.num_nodes());
    else if (name == "random_in_range")
        return detail::make_random_in_range_experiment(seed, num_queries, min_duration_graph,
                                                       min_consumption_graph, max_consumption_graph,
                                                       max_capacity, num_threads);
    else if (name == "random_in_charger_range")
        return detail::make_random_in_charger_range_experiment(
            seed, num_queries, min_duration_graph, min_consumption_graph, max_consumption_graph,
            chargers, max_capacity, num_threads);
    else if (name == "rank_in_charger_range")
        return detail::make_rank_in_charger_range_experiment(
            seed, num_queries, min_duration_graph, min_consumption_graph, max_consumption_graph,
            chargers, max_capacity, num_threads);
    else
        throw std::runtime_error("Invalid experiment name: " + name);
}
} // namespace charge::experiments

#endif
