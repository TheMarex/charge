#ifndef CHARGE_EV_NODE_POTENTIALS_HPP
#define CHARGE_EV_NODE_POTENTIALS_HPP

#include "common/dijkstra.hpp"
#include "ev/graph.hpp"
#include "ev/graph_transform.hpp"

namespace charge::ev {
class OmegaNodePotentials {
  public:
    using node_id_t = ev::DurationGraph::node_id_t;
    using key_t = std::int32_t;

    OmegaNodePotentials(const double capacity, const double min_charging_rate,
                        const DurationGraph &reverse_duration_graph,
                        const ConsumptionGraph &reverse_consumption_graph,
                        const OmegaGraph &reverse_omega_graph)
        : capacity(capacity), min_charging_rate(min_charging_rate),
          reverse_duration_graph(reverse_duration_graph),
          reverse_consumption_graph(reverse_consumption_graph),
          reverse_omega_graph(reverse_omega_graph),
          duration_to_landmark(reverse_duration_graph.num_nodes(), common::INF_WEIGHT),
          consumption_to_landmark(reverse_consumption_graph.num_nodes(), common::INF_WEIGHT),
          omega_to_landmark(reverse_omega_graph.num_nodes(), common::INF_WEIGHT) {}

    inline bool check_consitency(const node_id_t u, const node_id_t v,
                                 const ev::LimitedTradeoffFunction &f_uv) const {
        for (double y_su = 0.0; y_su < capacity; y_su += 100) {
            for (double x = f_uv.min_x; x < f_uv.max_x; x += 10) {
                auto y_uv = f_uv(x);
                auto omega_sv = omega(v, y_su + y_uv);
                auto omega_su = omega(u, y_su);
                // consistency is impared a littled since we use rounding
                if (common::to_upper_fixed(x) + omega_sv + common::to_fixed(0.001) < omega_su) {
                    return false;
                }
            }
        }
        return true;
    }

    inline key_t omega(const node_id_t node, const double consumption) const {
        const auto remaining_consumption = common::from_fixed(consumption_to_landmark[node]);

        if (remaining_consumption + consumption > capacity) {
            return omega_to_landmark[node] +
                   common::to_fixed((capacity - consumption) / min_charging_rate);
        } else {
            return duration_to_landmark[node];
        }
    }

    inline key_t key(const node_id_t node, const key_t,
                     const ev::PiecewieseTradeoffFunction &tradeoff) const {
        if (duration_to_landmark[node] == common::INF_WEIGHT)
            return common::INF_WEIGHT;

        const auto remaining_consumption = common::from_fixed(consumption_to_landmark[node]);
        const auto max_feasible_y = capacity - remaining_consumption;
        const auto max_y = tradeoff(tradeoff.min_x());

        // if we can drive on the fastest path, just do that
        if (max_feasible_y >= max_y) {
            return common::to_fixed(tradeoff.min_x()) + duration_to_landmark[node];
        }

        // we might need to use our tradeoffs to get to the target
        // compute the minimal time needed for that + how long it takes then
        auto min_feasible_x = tradeoff.inverse(max_feasible_y);
        assert(min_feasible_x >= tradeoff.min_x());

        key_t tradeoff_key = common::INF_WEIGHT;

        if (min_feasible_x < tradeoff.max_x()) {
            tradeoff_key = common::to_fixed(min_feasible_x) + duration_to_landmark[node];
        }

        auto min_charging_x =
            std::min(tradeoff.max_x(),
                     std::max(tradeoff.min_x(), tradeoff.inverse_deriv(min_charging_rate)));
        auto min_charging_y = tradeoff(min_charging_x);
        key_t charging_key = common::to_fixed(min_charging_x) + omega_to_landmark[node] +
                             common::to_fixed((capacity - min_charging_y) / min_charging_rate);

        auto key = std::min(charging_key, tradeoff_key);
        assert(key < common::INF_WEIGHT);

#ifndef NDEBUG
        // Consistent key check
        for (auto x = tradeoff.min_x(); x < tradeoff.max_x(); x += 10) {
            auto y = tradeoff(x);
            auto other_key = common::to_fixed(x) + omega(node, y);
            assert(other_key + common::to_fixed(0.001) >= key);
        }
#endif

        return key;
    }

    void recompute(common::MinIDQueue &queue, const node_id_t landmark) {
        dijkstra_to_all(landmark, reverse_duration_graph, queue, duration_to_landmark);
        dijkstra_to_all(landmark, reverse_consumption_graph, queue, consumption_to_landmark);
        dijkstra_to_all(landmark, reverse_omega_graph, queue, omega_to_landmark);
    }

  private:
    const double capacity;
    const double min_charging_rate;
    const DurationGraph &reverse_duration_graph;
    const ConsumptionGraph &reverse_consumption_graph;
    const OmegaGraph &reverse_omega_graph;
    common::CostVector<DurationGraph> duration_to_landmark;
    common::CostVector<ConsumptionGraph> consumption_to_landmark;
    common::CostVector<OmegaGraph> omega_to_landmark;
};

class LazyOmegaNodePotentials {
  public:
    using node_id_t = ev::DurationGraph::node_id_t;
    using key_t = std::int32_t;

    LazyOmegaNodePotentials(const double capacity, const double min_charging_rate,
                            const DurationGraph &reverse_duration_graph,
                            const ConsumptionGraph &reverse_consumption_graph,
                            const std::vector<std::int32_t> &shifted_consumption_potentials,
                            const OmegaGraph &reverse_omega_graph,
                            const std::vector<std::int32_t> &shifted_omega_potentials)
        : capacity(capacity), min_charging_rate(min_charging_rate),
          reverse_duration_graph(reverse_duration_graph),
          reverse_consumption_graph(reverse_consumption_graph),
          shifted_consumption_potentials(shifted_consumption_potentials),
          reverse_omega_graph(reverse_omega_graph),
          shifted_omega_potentials(shifted_omega_potentials),
          duration_to_landmark(reverse_duration_graph.num_nodes(), common::INF_WEIGHT),
          consumption_to_landmark(reverse_consumption_graph.num_nodes(), common::INF_WEIGHT),
          omega_to_landmark(reverse_omega_graph.num_nodes(), common::INF_WEIGHT),
          duration_queue(reverse_duration_graph.num_nodes()),
          consumption_queue(reverse_consumption_graph.num_nodes()),
          omega_queue(reverse_omega_graph.num_nodes()),
          duration_settled(reverse_duration_graph.num_nodes(), false),
          consumption_settled(reverse_consumption_graph.num_nodes(), false),
          omega_settled(reverse_omega_graph.num_nodes(), false) {}

    inline bool check_consitency(const node_id_t u, const node_id_t v,
                                 const ev::LimitedTradeoffFunction &f_uv) const {
        for (double y_su = 0.0; y_su < capacity; y_su += 100) {
            for (double x = f_uv.min_x; x < f_uv.max_x; x += 10) {
                auto y_uv = f_uv(x);
                auto omega_sv = omega(v, y_su + y_uv);
                auto omega_su = omega(u, y_su);
                // consistency is impared a littled since we use rounding
                if (common::to_upper_fixed(x) + omega_sv + common::to_fixed(0.001) < omega_su)
                    return false;
            }
        }
        return true;
    }

    inline key_t omega(const node_id_t node, const double consumption) const {
        const auto min_duration = continue_dijkstra(node, reverse_duration_graph, duration_queue,
                                                    duration_to_landmark, duration_settled);
        const auto min_shifted_consumption =
            continue_dijkstra(node, reverse_consumption_graph, consumption_queue,
                              consumption_to_landmark, consumption_settled);
        const auto min_consumption = min_shifted_consumption -
                                     shifted_consumption_potentials[node] +
                                     shifted_consumption_potentials[landmark];
        const auto min_shifted_omega = continue_dijkstra(node, reverse_omega_graph, omega_queue,
                                                         omega_to_landmark, omega_settled);
        const auto min_omega =
            min_shifted_omega - shifted_omega_potentials[node] + shifted_omega_potentials[landmark];

        const auto remaining_consumption = common::from_fixed(min_consumption);

        if (remaining_consumption + consumption > capacity) {
            return min_omega + common::to_fixed((capacity - consumption) / min_charging_rate);
        } else {
            return min_duration;
        }
    }

    inline key_t key(const node_id_t node, const key_t,
                     const ev::PiecewieseTradeoffFunction &tradeoff) const {
        const auto min_duration = continue_dijkstra(node, reverse_duration_graph, duration_queue,
                                                    duration_to_landmark, duration_settled);

        if (min_duration == common::INF_WEIGHT)
            return common::INF_WEIGHT;

        const auto min_shifted_consumption =
            continue_dijkstra(node, reverse_consumption_graph, consumption_queue,
                              consumption_to_landmark, consumption_settled);
        const auto min_consumption = min_shifted_consumption -
                                     shifted_consumption_potentials[node] +
                                     shifted_consumption_potentials[landmark];
        assert(consumption_settled[node]);

        const auto remaining_consumption = common::from_fixed(min_consumption);
        const auto max_feasible_y = capacity - remaining_consumption;
        const auto min_x = tradeoff.min_x();
        const auto max_y = tradeoff(min_x);

        // if we can drive on the fastest path, just do that
        if (max_feasible_y >= max_y) {
            return common::to_fixed(tradeoff.min_x()) + min_duration;
        }


        // we might need to use our tradeoffs to get to the target
        // compute the minimal time needed for that + how long it takes then
        auto min_feasible_x = tradeoff.inverse(max_feasible_y);
        assert(min_feasible_x >= min_x);

        key_t tradeoff_key = common::INF_WEIGHT;

        const auto max_x = tradeoff.max_x();
        if (min_feasible_x < max_x) {
            tradeoff_key = common::to_fixed(min_feasible_x) + min_duration;
        }

        key_t charging_key = common::INF_WEIGHT;
        auto min_charging_x =
            std::min(max_x, std::max(min_x, tradeoff.inverse_deriv(min_charging_rate)));
        auto min_charging_y = tradeoff(min_charging_x);
        // FIXME enable once measurements are done, this give a slight speedup
        //if (min_charging_y > max_feasible_y)
        //{
            auto min_shifted_omega = continue_dijkstra(node, reverse_omega_graph, omega_queue,
                                                       omega_to_landmark, omega_settled);
            const auto min_omega =
                min_shifted_omega - shifted_omega_potentials[node] + shifted_omega_potentials[landmark];
            assert(omega_settled[node]);

            charging_key = common::to_fixed(min_charging_x) + min_omega +
                                 common::to_fixed((capacity - min_charging_y) / min_charging_rate);
        //}

        auto key = std::min(charging_key, tradeoff_key);
        assert(key < common::INF_WEIGHT);

#ifndef NDEBUG
        // Consistent key check
        for (auto x = tradeoff.min_x(); x < tradeoff.max_x(); x += 10) {
            auto y = tradeoff(x);
            auto other_key = common::to_fixed(x) + omega(node, y);
            assert(other_key + common::to_fixed(0.001) >= key);
        }
#endif

        return key;
    }

    void recompute(const node_id_t source, const node_id_t target) {
        landmark = target;
        dijkstra(target, source, reverse_duration_graph, duration_queue, duration_to_landmark,
                 duration_settled);
        dijkstra(target, source, reverse_consumption_graph, consumption_queue,
                 consumption_to_landmark, consumption_settled);
        dijkstra(target, source, reverse_omega_graph, omega_queue, omega_to_landmark,
                 omega_settled);
    }

  private:
    const double capacity;
    const double min_charging_rate;
    const DurationGraph &reverse_duration_graph;
    const ConsumptionGraph &reverse_consumption_graph;
    const std::vector<std::int32_t> &shifted_consumption_potentials;
    const OmegaGraph &reverse_omega_graph;
    const std::vector<std::int32_t> &shifted_omega_potentials;
    mutable node_id_t landmark;
    mutable common::CostVector<DurationGraph> duration_to_landmark;
    mutable common::CostVector<ConsumptionGraph> consumption_to_landmark;
    mutable common::CostVector<OmegaGraph> omega_to_landmark;
    mutable common::MinIDQueue duration_queue;
    mutable common::MinIDQueue consumption_queue;
    mutable common::MinIDQueue omega_queue;
    mutable std::vector<bool> duration_settled;
    mutable std::vector<bool> consumption_settled;
    mutable std::vector<bool> omega_settled;
};
} // namespace charge::ev

#endif
