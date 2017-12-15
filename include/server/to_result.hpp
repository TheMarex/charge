#ifndef CHARGE_SERVER_TO_RESULT_HPP
#define CHARGE_SERVER_TO_RESULT_HPP

#include "common/path.hpp"

#include "ev/fp_dijkstra.hpp"
#include "ev/mc_dijkstra.hpp"

#include "server/route_result.hpp"

namespace charge::server {

inline void annotate_heights(RouteResult &route, const std::vector<std::int32_t> &heights) {
    route.heights.resize(route.path.size());
    std::transform(route.path.begin(), route.path.end(), route.heights.begin(),
                   [&](const auto id) { return heights[id]; });
}

inline void annotate_coordinates(RouteResult &route,
                                 const std::vector<common::Coordinate> &coordinates) {
    route.geometry.resize(route.path.size());
    std::transform(route.path.begin(), route.path.end(), route.geometry.begin(),
                   [&](const auto id) { return coordinates[id]; });
}

// needs to be called after annotating geometry
inline void annotate_lengths(RouteResult &route) {
    double length = 0;
    route.lengths.push_back(0);
    std::adjacent_find(route.geometry.begin(), route.geometry.end(),
                       [&length, &route](const auto &lhs, const auto &rhs) {
                           length += common::haversine_distance(lhs, rhs);
                           route.lengths.push_back(length);
                           return false;
                       });
}

// needs to be called after annotating length
inline void annotate_max_speeds(RouteResult &route, const ev::TradeoffGraph& graph) {
    for (auto index = 0u; index < route.path.size() - 1; ++index)
    {
        auto start = route.path[index];
        auto target = route.path[index+1];
        if (start == target)
        {
            route.max_speeds.push_back(0);
            continue;
        }
        auto length = route.lengths[index+1] - route.lengths[index];
        auto edge = graph.edge(start, target);
        const auto& cost = graph.weight(edge);
        route.max_speeds.push_back(length / cost.min_x * 3.6);
    }
}

template <typename NodeLabelsT>
RouteResult to_result(ev::TradeoffGraph::node_id_t start, ev::TradeoffGraph::node_id_t target,
                      const ev::TradeoffLabelEntryWithParent &last_label, const NodeLabelsT &labels) {
    RouteResult route;
    route.tradeoff = last_label.cost;
    auto[path, path_labels] = common::get_path_with_labels(start, target, last_label, labels);
    route.path = std::move(path);
    route.durations.reserve(route.path.size());
    route.consumptions.reserve(route.path.size());

    // minimal total time
    auto current_duration = last_label.cost.min_x();
    for (int index = path_labels.size() - 1; index >= 0; index--) {
        const auto &label = path_labels[index];
        route.durations.push_back(current_duration);
        route.consumptions.push_back(label.cost(std::max(label.cost.min_x(), current_duration)));
        // we need to take the maximum in case we run into numeric issues
        // around the minumum
        current_duration = label.delta(std::max(label.delta.min_x(), current_duration));
        assert(std::isfinite(current_duration));
    }
    std::reverse(route.consumptions.begin(), route.consumptions.end());
    std::reverse(route.durations.begin(), route.durations.end());

    return route;
}

template <typename NodeLabelsT>
RouteResult to_result(ev::DurationConsumptionGraph::node_id_t start,
                      ev::DurationConsumptionGraph::node_id_t target,
                      const ev::DurationConsumptionLabelEntryWithParent &label,
                      const NodeLabelsT &labels) {
    RouteResult route;
    auto duration = common::from_fixed(std::get<0>(label.cost));
    auto consumption = common::from_fixed(std::get<1>(label.cost));
    route.tradeoff = ev::make_constant(duration, consumption);
    auto[path, path_labels] = common::get_path_with_labels(start, target, label, labels);
    route.path = std::move(path);
    route.durations.reserve(route.path.size());
    route.consumptions.reserve(route.path.size());
    for (const auto &label : path_labels) {
        route.durations.push_back(common::from_fixed(std::get<0>(label.cost)));
        route.consumptions.push_back(common::from_fixed(std::get<1>(label.cost)));
    }
    return route;
}

inline RouteResult to_result(ev::DurationGraph::node_id_t start,
                             ev::DurationGraph::node_id_t target,
                             const ev::TradeoffGraph &tradeoff_graph,
                             const common::CostVector<ev::DurationGraph> &costs,
                             const common::ParentVector<ev::DurationGraph> &parents) {

    RouteResult route;
    std::vector<ev::DurationGraph::weight_t> path_costs;
    std::tie(route.path, path_costs) = common::get_path_with_labels<ev::DurationGraph>(
        start, target, parents, costs);
    route.durations.resize(path_costs.size());
    std::transform(path_costs.begin(), path_costs.end(), route.durations.begin(),
                   [](const auto cost) { return common::from_fixed(cost); });

    auto total_consumption = 0;
    route.consumptions.push_back(total_consumption);
    for (auto index : common::irange<std::size_t>(0, route.path.size() - 1)) {
        auto current = route.path[index];
        auto next = route.path[index + 1];
        auto duration = route.durations[index + 1] - route.durations[index];
        auto edge = tradeoff_graph.edge(current, next);
        assert(edge != common::INVALID_ID);
        const auto &tradeoff = tradeoff_graph.weight(edge);

        assert(tradeoff.min_x <= duration + 0.1);
        auto consumption = tradeoff(std::max(duration, tradeoff.min_x));
        total_consumption += consumption;

        route.consumptions.push_back(total_consumption);
    }

    route.tradeoff = ev::make_constant(route.durations.back(), route.consumptions.back());
    return route;
}
}

#endif
