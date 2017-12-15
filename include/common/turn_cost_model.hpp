#ifndef CHARGE_COMMON_TURN_COST_MODEL_HPP
#define CHARGE_COMMON_TURN_COST_MODEL_HPP

#include "common/coordinate.hpp"
#include "common/function_graph.hpp"
#include "common/hyp_lin_function.hpp"

#include <memory>

namespace charge::common {

namespace detail {
inline double brake_time(const double from_speed, const double to_speed) {
    if (from_speed <= to_speed)
        return 0;
    assert(from_speed > to_speed);

    constexpr double CAR_MAX_BRAKE_DEACCELERATION = 8; // m/s^2 (on dry ground)

    return (from_speed - to_speed) / CAR_MAX_BRAKE_DEACCELERATION;
}

inline double brake_consumption(const double from_speed, const double to_speed) {
    if (from_speed <= to_speed)
        return 0;
    assert(from_speed > to_speed);

    static constexpr double MASS = 1080;         // mass in kg of a Pegeaut IOn
    static constexpr double RECUPERTATION = 0.3; // Percentage of energy that gained by braking
    static constexpr double Ws2Wh = 1.0 / 60.0 / 60.0;

    double energy =
        0.5 * MASS * (from_speed * from_speed - to_speed * to_speed) * RECUPERTATION * Ws2Wh;

    // we gain some energy
    return -energy;
}

inline double speedup_consumption(const double from_speed, const double to_speed) {
    if (from_speed >= to_speed)
        return 0;
    assert(from_speed < to_speed);

    static constexpr double MASS = 1080; // mass in kg of a Pegeaut IOn
    static constexpr double EFFICIENCY =
        0.8; // Percentage of energy output for every watt put in the motor
    static constexpr double Ws2Wh = 1.0 / 60.0 / 60.0;

    double energy =
        0.5 * MASS * (to_speed * to_speed - from_speed * from_speed) / EFFICIENCY * Ws2Wh;

    // we loose some energy
    return energy;
}

inline double speedup_time(const double from_speed, const double to_speed) {
    if (from_speed >= to_speed)
        return 0;
    assert(from_speed < to_speed);

    constexpr double CAR_MAX_ACCELERATION = 2.7; // m/s^2 for a Pegeaut IOn

    return (to_speed - from_speed) / CAR_MAX_ACCELERATION;
}

inline double brake_length(const double from_speed, const double to_speed) {
    if (from_speed <= to_speed)
        return 0;
    assert(from_speed > to_speed);

    const auto dt = brake_time(from_speed, to_speed);

    return dt * (from_speed + 0.5 * (to_speed - from_speed));
}

inline double speedup_length(const double from_speed, const double to_speed) {
    if (from_speed >= to_speed)
        return 0;
    assert(from_speed < to_speed);

    const auto dt = speedup_time(from_speed, to_speed);

    return dt * (from_speed + 0.5 * (to_speed - from_speed));
}

} // namespace detail

template <typename GraphT> class TurnCostModel {
  public:
    static constexpr double UTURN_COST = 20;
    virtual typename GraphT::weight_t operator()(const GraphT &graph, const std::size_t degree,
                                                 typename GraphT::node_id_t from,
                                                 typename GraphT::node_id_t via,
                                                 typename GraphT::node_id_t to) const = 0;
};

template <typename GraphT> class ZeroTurnCostModel final : public TurnCostModel<GraphT> {
  public:
    virtual typename GraphT::weight_t
    operator()(const GraphT &graph, const std::size_t, typename GraphT::node_id_t from,
               typename GraphT::node_id_t via, typename GraphT::node_id_t to) const override final {
        if (from == to) {
            auto cost = graph.weight(graph.edge(from, via));
            cost.shift(TurnCostModel<GraphT>::UTURN_COST);
            return cost;
        } else {
            return graph.weight(graph.edge(from, via));
        }
    }
};

template <typename GraphT> class StaticTurnCostModel;

template <typename FnT>
class StaticTurnCostModel<FunctionGraph<FnT>> final : public TurnCostModel<FunctionGraph<FnT>> {
  public:
    static constexpr double STATIC_COST = 7.5;
    using GraphT = FunctionGraph<FnT>;

    StaticTurnCostModel(const std::vector<common::Coordinate>& coordinates) : coordinates{coordinates} {}

    virtual typename GraphT::weight_t
    operator()(const GraphT &graph, const std::size_t degree, typename GraphT::node_id_t from,
               typename GraphT::node_id_t via, typename GraphT::node_id_t to) const override final {
        auto cost = graph.weight(graph.edge(from, via));
        if (degree > 2)
        {
            auto from_bearing = bearing(coordinates[via], coordinates[from]);
            auto to_bearing = bearing(coordinates[via], coordinates[to]);
            auto angle = from_bearing - to_bearing;
            if (angle < -180)
                angle += 360;
            else if (angle > 180)
                angle -= 360;

            // Emperical turn penalty function
            double penalty = STATIC_COST / (1 + std::exp( -(13 * (180-std::fabs(angle))/180 - 6.5)));

            cost.shift(penalty);
        }
        if (from == to)
            cost.shift(TurnCostModel<GraphT>::UTURN_COST);

        return cost;
    }

    const std::vector<Coordinate> &coordinates;
};

template <typename FnT> class PhysicalTurnCostModel;

// Arrives with maximum speed and leaves with minimum speed
template <> class PhysicalTurnCostModel<HypOrLinFunction> {
  protected:
    using GraphT = FunctionGraph<HypOrLinFunction>;

    PhysicalTurnCostModel(const std::vector<Coordinate> &coordinates) : coordinates(coordinates) {}

    typename GraphT::weight_t compute_cost(const typename GraphT::weight_t &from_cost,
                                           const typename GraphT::weight_t &,
                                           const double from_duration, const double to_duration,
                                           typename GraphT::node_id_t from,
                                           typename GraphT::node_id_t via,
                                           typename GraphT::node_id_t to) const {

        auto from_length = haversine_distance(coordinates[from], coordinates[via]);
        auto to_length = haversine_distance(coordinates[via], coordinates[to]);

        auto arrive_speed = from_length / from_duration;
        auto leave_speed = to_length / to_duration;

        auto cost = from_cost;
        if (std::fabs(arrive_speed - leave_speed) > 1) {

            auto time_penalty = detail::speedup_time(arrive_speed, leave_speed) +
                                detail::brake_time(arrive_speed, leave_speed);

            auto transition_length = detail::speedup_length(arrive_speed, leave_speed) +
                                     detail::brake_length(arrive_speed, leave_speed);

            auto consumption_penalty = detail::speedup_consumption(arrive_speed, leave_speed) +
                                       detail::brake_consumption(arrive_speed, leave_speed);

            auto alpha = std::max(0.0, 1.0 - transition_length / from_length);

            // the segment where we could to the tradeoff is shorter
            // then the full segment because the speed is constrained
            cost.min_x *= alpha;
            cost.max_x *= alpha;
            if (cost->is_hyperbolic()) {
                auto &hyp = static_cast<HyperbolicFunction &>(*cost);
                hyp.a *= alpha * alpha * alpha;
                hyp.c *= alpha;
            }

            cost.shift(time_penalty);
            cost.offset(consumption_penalty);
        }

        return cost;
    }

    const std::vector<Coordinate> &coordinates;
};

template <typename GraphT> class MaxTurnCostModel;

// Assumes we drive at the maximum speed on both edges
template <typename FnT>
class MaxTurnCostModel<FunctionGraph<FnT>> final : public TurnCostModel<FunctionGraph<FnT>>,
                                                   private PhysicalTurnCostModel<FnT> {
  public:
    MaxTurnCostModel(const std::vector<Coordinate> &coordinates)
        : PhysicalTurnCostModel<FnT>{coordinates} {}

    using GraphT = FunctionGraph<FnT>;
    virtual typename GraphT::weight_t
    operator()(const GraphT &graph, const std::size_t, typename GraphT::node_id_t from,
               typename GraphT::node_id_t via, typename GraphT::node_id_t to) const override final {
        const auto &from_cost = graph.weight(graph.edge(from, via));
        const auto &to_cost = graph.weight(graph.edge(via, to));

        auto from_duration = from_cost.min_x;
        auto to_duration = to_cost.min_x;

        auto cost =
            this->compute_cost(from_cost, to_cost, from_duration, to_duration, from, via, to);
        if (from == to)
            cost.shift(TurnCostModel<GraphT>::UTURN_COST);

        return cost;
    }
};

template <typename GraphT> class AvgConsumptionTurnCostModel;
// Assumes we drive at the maximum speed on both edges
template <typename FnT>
class AvgConsumptionTurnCostModel<FunctionGraph<FnT>>
    : public TurnCostModel<FunctionGraph<FnT>> {
  public:
    AvgConsumptionTurnCostModel(const std::vector<Coordinate> &coordinates)
        : coordinates{coordinates} {}

    using GraphT = FunctionGraph<FnT>;
    virtual typename GraphT::weight_t
    operator()(const GraphT &graph, const std::size_t, typename GraphT::node_id_t from,
               typename GraphT::node_id_t via, typename GraphT::node_id_t to) const override final {
        const auto &from_cost = graph.weight(graph.edge(from, via));
        const auto &to_cost = graph.weight(graph.edge(via, to));

        auto min_from_duration = from_cost.min_x;
        auto max_from_duration = from_cost.max_x;
        auto min_to_duration = to_cost.min_x;
        auto max_to_duration = to_cost.max_x;

        auto from_length = haversine_distance(coordinates[from], coordinates[via]);
        auto to_length = haversine_distance(coordinates[via], coordinates[to]);

        auto min_arrive_speed = from_length / min_from_duration;
        auto min_leave_speed = to_length / min_to_duration;
        auto max_arrive_speed = from_length / max_from_duration;
        auto max_leave_speed = to_length / max_to_duration;

        auto cost = from_cost;
        auto min_min_consumption_penalty =
            detail::speedup_consumption(min_arrive_speed, min_leave_speed) +
            detail::brake_consumption(min_arrive_speed, min_leave_speed);
        auto min_max_consumption_penalty =
            detail::speedup_consumption(min_arrive_speed, max_leave_speed) +
            detail::brake_consumption(min_arrive_speed, max_leave_speed);
        auto max_min_consumption_penalty =
            detail::speedup_consumption(max_arrive_speed, min_leave_speed) +
            detail::brake_consumption(max_arrive_speed, min_leave_speed);
        auto max_max_consumption_penalty =
            detail::speedup_consumption(max_arrive_speed, max_leave_speed) +
            detail::brake_consumption(max_arrive_speed, max_leave_speed);

        auto max_consumption_penalty =
            std::max(std::max(min_min_consumption_penalty, min_max_consumption_penalty),
                     std::max(max_min_consumption_penalty, max_max_consumption_penalty));
        auto min_consumption_penalty =
            std::min(std::min(min_min_consumption_penalty, min_max_consumption_penalty),
                     std::min(max_min_consumption_penalty, max_max_consumption_penalty));

        cost.offset((max_consumption_penalty + min_consumption_penalty) / 2.0);

        if (from == to)
            cost.shift(TurnCostModel<GraphT>::UTURN_COST);

        return cost;
    }
    const std::vector<Coordinate> &coordinates;
};

template <typename GraphT> class AvgConsumptionStaticTurnCostModel;
// Assumes we drive at the maximum speed on both edges
template <typename FnT>
class AvgConsumptionStaticTurnCostModel<FunctionGraph<FnT>> final
    : public TurnCostModel<FunctionGraph<FnT>> {
  public:
    AvgConsumptionStaticTurnCostModel(const std::vector<Coordinate> &coordinates)
        : coordinates{coordinates} {}

    static constexpr double STATIC_COST = 7.5;

    using GraphT = FunctionGraph<FnT>;
    virtual typename GraphT::weight_t
    operator()(const GraphT &graph, const std::size_t degree, typename GraphT::node_id_t from,
               typename GraphT::node_id_t via, typename GraphT::node_id_t to) const override final {
        const auto &from_cost = graph.weight(graph.edge(from, via));
        const auto &to_cost = graph.weight(graph.edge(via, to));

        auto min_from_duration = from_cost.min_x;
        auto max_from_duration = from_cost.max_x;
        auto min_to_duration = to_cost.min_x;
        auto max_to_duration = to_cost.max_x;

        auto from_length = haversine_distance(coordinates[from], coordinates[via]);
        auto to_length = haversine_distance(coordinates[via], coordinates[to]);

        auto min_arrive_speed = from_length / min_from_duration;
        auto min_leave_speed = to_length / min_to_duration;
        auto max_arrive_speed = from_length / max_from_duration;
        auto max_leave_speed = to_length / max_to_duration;

        auto cost = from_cost;
        auto min_min_consumption_penalty =
            detail::speedup_consumption(min_arrive_speed, min_leave_speed) +
            detail::brake_consumption(min_arrive_speed, min_leave_speed);
        auto min_max_consumption_penalty =
            detail::speedup_consumption(min_arrive_speed, max_leave_speed) +
            detail::brake_consumption(min_arrive_speed, max_leave_speed);
        auto max_min_consumption_penalty =
            detail::speedup_consumption(max_arrive_speed, min_leave_speed) +
            detail::brake_consumption(max_arrive_speed, min_leave_speed);
        auto max_max_consumption_penalty =
            detail::speedup_consumption(max_arrive_speed, max_leave_speed) +
            detail::brake_consumption(max_arrive_speed, max_leave_speed);

        auto max_consumption_penalty =
            std::max(std::max(min_min_consumption_penalty, min_max_consumption_penalty),
                     std::max(max_min_consumption_penalty, max_max_consumption_penalty));
        auto min_consumption_penalty =
            std::min(std::min(min_min_consumption_penalty, min_max_consumption_penalty),
                     std::min(max_min_consumption_penalty, max_max_consumption_penalty));

        cost.offset((max_consumption_penalty + min_consumption_penalty) / 2.0);

        if (degree > 2)
        {
            auto from_bearing = bearing(coordinates[via], coordinates[from]);
            auto to_bearing = bearing(coordinates[via], coordinates[to]);
            auto angle = from_bearing - to_bearing;
            if (angle < -180)
                angle += 360;
            else if (angle > 180)
                angle -= 360;

            // Emperical turn penalty function
            double penalty = STATIC_COST / (1 + std::exp( -(13 * (180-std::fabs(angle))/180 - 6.5)));

            cost.shift(penalty);
        }

        return cost;
    }
    const std::vector<Coordinate> &coordinates;
};

template <typename GraphT>
auto make_turn_cost_model(const std::string &name, const std::vector<Coordinate> &coordinates) {
    if (name == "zero")
        return std::dynamic_pointer_cast<TurnCostModel<GraphT>>(
            std::make_shared<ZeroTurnCostModel<GraphT>>());
    if (name == "static")
        return std::dynamic_pointer_cast<TurnCostModel<GraphT>>(
            std::make_shared<StaticTurnCostModel<GraphT>>(coordinates));
    if (name == "max")
        return std::dynamic_pointer_cast<TurnCostModel<GraphT>>(
            std::make_shared<MaxTurnCostModel<GraphT>>(coordinates));
    if (name == "avg_consumption")
        return std::dynamic_pointer_cast<TurnCostModel<GraphT>>(
            std::make_shared<AvgConsumptionTurnCostModel<GraphT>>(coordinates));
    if (name == "avg_consumption_static")
        return std::dynamic_pointer_cast<TurnCostModel<GraphT>>(
            std::make_shared<AvgConsumptionStaticTurnCostModel<GraphT>>(coordinates));
    else
        throw std::runtime_error("Unknown turn cost model " + name);
}
} // namespace charge::common

#endif
