#ifndef CHARGE_EV_GRAPH_HPP
#define CHARGE_EV_GRAPH_HPP

#include "common/weighted_graph.hpp"

#include "ev/limited_tradeoff_function.hpp"

namespace charge::ev {

class TradeoffGraph : public common::WeightedGraph<ev::LimitedTradeoffFunction> {
  public:
    using Base = common::WeightedGraph<ev::LimitedTradeoffFunction>;
    using Base::Base;

    TradeoffGraph(Base &&base) : Base(std::forward<Base>(base)) {}

    double min_tradeoff_rate() const
    {
        double min_tradeoff_rate = std::numeric_limits<double>::infinity();
        for (const auto edge : common::irange<Base::edge_id_t>(0, num_edges()))
        {
            const auto& tradeoff = weight(edge);
            min_tradeoff_rate = std::min(tradeoff->deriv(tradeoff.min_x), min_tradeoff_rate);
        }

        return min_tradeoff_rate;
    }
};

class DurationConsumptionGraph
    : public common::WeightedGraph<std::tuple<std::int32_t, std::int32_t>> {
  public:
    using Base = common::WeightedGraph<std::tuple<std::int32_t, std::int32_t>>;
    using Base::Base;

    DurationConsumptionGraph(Base &&base) : Base(std::forward<Base>(base)) {}
};

class OmegaGraph : public common::WeightedGraph<std::int32_t> {
  public:
    using Base = common::WeightedGraph<std::int32_t>;
    using Base::Base;

    OmegaGraph(Base &&base) : Base(std::forward<Base>(base)) {}
};

class DurationGraph : public common::WeightedGraph<std::int32_t> {
  public:
    using Base = common::WeightedGraph<std::int32_t>;
    using Base::Base;

    DurationGraph(Base &&base) : Base(std::forward<Base>(base)) {}
};

class ConsumptionGraph : public common::WeightedGraph<std::int32_t> {
  public:
    using Base = common::WeightedGraph<std::int32_t>;
    using Base::Base;

    ConsumptionGraph(Base &&base) : Base(std::forward<Base>(base)) {}
};
}

#endif
