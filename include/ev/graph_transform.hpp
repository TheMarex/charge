#ifndef CHARGE_EV_GRAPH_TRANSFORM_HPP
#define CHARGE_EV_GRAPH_TRANSFORM_HPP

#include "common/coordinate.hpp"
#include "common/sample_function.hpp"
#include "common/sink_iter.hpp"

#include "ev/graph.hpp"

#include <cmath>
#include <functional>

namespace charge::ev {
inline auto tradeoff_to_min_duration(TradeoffGraph tradeoff_graph_) {
    using OutGraph = DurationGraph;
    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph_));

    std::vector<std::int32_t> durations(tradeoffs.size());
    std::transform(tradeoffs.begin(), tradeoffs.end(), durations.begin(),
                   [&](const auto &tradeoff) {
                       return std::ceil(tradeoff.min_x * common::FIXED_POINT_RESOLUTION);
                   });

    return OutGraph(std::move(first_out), std::move(heads), std::move(durations));
}

template <typename GraphT>
auto shift_negative_weights(GraphT &graph, const std::vector<std::int32_t> &heights) {
    double lower_alpha = 0;
    double upper_alpha = std::numeric_limits<double>::infinity();
    for (const auto start : graph.nodes()) {
        for (const auto edge : graph.edges(start)) {
            auto consumption = graph.weight(edge);
            auto target = graph.target(edge);
            auto dh = heights[target] - heights[start];

            if (dh != 0) {
                double new_alpha = consumption / dh;

                if (dh < 0 && consumption < 0) {
                    lower_alpha = std::max(lower_alpha, new_alpha);
                } else if (dh > 0) {
                    assert(consumption >= 0);
                    assert(new_alpha >= 0);
                    upper_alpha = std::min(upper_alpha, new_alpha);
                }
            }
        }
    }
    assert(lower_alpha < upper_alpha);
    auto alpha = lower_alpha + 1;

    std::vector<std::int32_t> shifting_potential = heights;
    for (auto &s : shifting_potential) {
        s = s * alpha;
    }

    for (const auto start : graph.nodes()) {
        for (const auto edge : graph.edges(start)) {
            auto &weight = graph.weight(edge);
            auto target = graph.target(edge);
            auto shifted_weight = weight + shifting_potential[start] - shifting_potential[target];
            if (shifted_weight < 0) {
                throw std::runtime_error("The graph has negative cycles: " + std::to_string(alpha) +
                                         ", " + std::to_string(shifted_weight));
            }
            weight = shifted_weight;
        }
    }

    return shifting_potential;
}

inline auto tradeoff_to_min_consumption(TradeoffGraph tradeoff_graph_) {
    using OutGraph = ConsumptionGraph;
    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph_));

    std::vector<std::int32_t> durations(tradeoffs.size());
    std::transform(tradeoffs.begin(), tradeoffs.end(), durations.begin(),
                   [&](const auto &tradeoff) {
                       return std::ceil(tradeoff(tradeoff.max_x) * common::FIXED_POINT_RESOLUTION);
                   });

    return OutGraph(std::move(first_out), std::move(heads), std::move(durations));
}

inline auto tradeoff_to_max_consumption(TradeoffGraph tradeoff_graph_) {
    using OutGraph = ConsumptionGraph;
    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph_));

    std::vector<std::int32_t> durations(tradeoffs.size());
    std::transform(tradeoffs.begin(), tradeoffs.end(), durations.begin(),
                   [&](const auto &tradeoff) {
                       return std::ceil(tradeoff(tradeoff.min_x) * common::FIXED_POINT_RESOLUTION);
                   });

    return OutGraph(std::move(first_out), std::move(heads), std::move(durations));
}

// Samples the tradeoff functions in `consumption_resolution` steps and create an edge
// for each. The result will be a multi-graph and not a normal graph, though the
// adjacency graph should handle this just fine.
using DurationAndConsumption = std::tuple<std::int32_t, std::int32_t>;
inline auto tradeoff_to_sampled_consumption(const TradeoffGraph &tradeoff_graph,
                                            const double consumption_resolution) {
    using OutGraph = DurationConsumptionGraph;

    std::vector<OutGraph::edge_t> edges;

    constexpr double MIN_DURATION_RESOLUTION = 0.01;

    for (const auto edge : tradeoff_graph.edges()) {
        const auto insert_edge = [&](const auto &sample) {
            const auto[sampled_time, sampled_consumption] = sample;
            edges.push_back(
                OutGraph::edge_t{edge.start, edge.target,
                                 DurationAndConsumption{sampled_time, sampled_consumption}});
        };

        common::sample_function(edge.weight, MIN_DURATION_RESOLUTION, consumption_resolution,
                                common::make_sink_iter(std::ref(insert_edge)));
    }

    return OutGraph{tradeoff_graph.num_nodes(), edges};
}

inline auto tradeoff_to_omega_graph(TradeoffGraph tradeoff_graph_, const double min_charging_rate) {
    using OutGraph = OmegaGraph;
    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph_));

    std::vector<std::int32_t> durations(tradeoffs.size());
    std::transform(tradeoffs.begin(), tradeoffs.end(), durations.begin(),
                   [&](const auto &tradeoff) {
                       if (tradeoff->is_linear()) {
                           const auto &lin = static_cast<const common::LinearFunction &>(*tradeoff);
                           if (lin.d > min_charging_rate) {
                               auto consumption = lin(tradeoff.max_x);
                               auto omega = tradeoff.max_x - consumption / min_charging_rate;
                               return common::to_fixed(omega);
                           } else {
                               assert(lin.d <= min_charging_rate);
                               auto consumption = lin(tradeoff.min_x);
                               auto omega = tradeoff.min_x - consumption / min_charging_rate;
                               return common::to_fixed(omega);
                           }
                       }

                       assert(tradeoff->is_hyperbolic());
                       const auto &hyp = static_cast<const common::HyperbolicFunction &>(*tradeoff);
                       auto x = hyp.inverse_deriv(min_charging_rate);
                       x = std::min(tradeoff.max_x, std::max(tradeoff.min_x, x));
                       auto consumption = hyp(x);
                       assert(std::isfinite(consumption));
                       assert(std::isfinite(x));
                       auto omega = x - consumption / min_charging_rate;
                       return common::to_fixed(omega);
                   });

    return OutGraph(std::move(first_out), std::move(heads), std::move(durations));
}

inline auto tradeoff_to_only_fast(TradeoffGraph tradeoff_graph,
                                  const std::vector<common::Coordinate> &coordinates) {
    std::vector<ev::LimitedTradeoffFunction> filtered_tradeoffs;

    for (auto node : tradeoff_graph.nodes()) {
        for (auto edge : tradeoff_graph.edges(node)) {
            auto target = tradeoff_graph.target(edge);
            auto length = common::haversine_distance(coordinates[node], coordinates[target]);
            auto cost = tradeoff_graph.weight(edge);
            auto max_speed_kmh = length / cost.min_x * 3.6;
            if (max_speed_kmh > 100) {
                cost.max_x = std::min(cost.max_x, length / (100 / 3.6));
                assert(cost.min_x <= cost.max_x);
            } else {
                cost = ev::LimitedTradeoffFunction{cost.min_x, cost.min_x,
                                                   common::ConstantFunction{cost(cost.min_x)}};
            }
            filtered_tradeoffs.push_back(cost);
        }
    }

    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph));
    assert(filtered_tradeoffs.size() == tradeoffs.size());
    return ev::TradeoffGraph(std::move(first_out), std::move(heads), std::move(filtered_tradeoffs));
}

inline auto tradeoff_to_linear(TradeoffGraph tradeoff_graph) {
    std::vector<ev::LimitedTradeoffFunction> filtered_tradeoffs;

    for (auto node : tradeoff_graph.nodes()) {
        for (auto edge : tradeoff_graph.edges(node)) {
            auto cost = tradeoff_graph.weight(edge);
            if (cost->is_hyperbolic()) {
                auto min_y = cost(cost.max_x);
                auto max_y = cost(cost.min_x);
                auto slope = (min_y - max_y) / (cost.max_x - cost.min_x);
                cost = ev::LimitedTradeoffFunction{
                    cost.min_x, cost.max_x, common::LinearFunction{slope, cost.min_x, max_y}};
            }
            filtered_tradeoffs.push_back(cost);
        }
    }

    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph));
    assert(filtered_tradeoffs.size() == tradeoffs.size());
    return ev::TradeoffGraph(std::move(first_out), std::move(heads), std::move(filtered_tradeoffs));
}

inline auto tradeoff_to_limited_rate(TradeoffGraph tradeoff_graph, const double min_rate) {
    std::vector<ev::LimitedTradeoffFunction> filtered_tradeoffs;

    for (auto node : tradeoff_graph.nodes()) {
        for (auto edge : tradeoff_graph.edges(node)) {
            auto cost = tradeoff_graph.weight(edge);
            if (cost->is_hyperbolic()) {
                const auto &hyp = static_cast<const common::HyperbolicFunction &>(*cost);
                auto x = hyp.inverse_deriv(min_rate);

                // make a constant function
                if (x < cost.min_x) {
                    cost = ev::LimitedTradeoffFunction{cost.min_x, cost.min_x,
                                                       common::ConstantFunction{cost(cost.min_x)}};
                }
                // we need to clip
                else if (x < cost.max_x) {
                    cost.max_x = x;
                }
            }
            filtered_tradeoffs.push_back(cost);
        }
    }

    auto[first_out, heads, tradeoffs] = TradeoffGraph::unwrap(std::move(tradeoff_graph));
    assert(filtered_tradeoffs.size() == tradeoffs.size());
    return ev::TradeoffGraph(std::move(first_out), std::move(heads), std::move(filtered_tradeoffs));
}

} // namespace charge::ev

#endif
