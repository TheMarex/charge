#ifndef CHARGE_COMMON_EDGE_HPP
#define CHARGE_COMMON_EDGE_HPP

#include "common/constants.hpp"

#include <tuple>

namespace charge {
namespace common {
template <typename NodeT, typename WeightT>
struct Edge {
    using node_t = NodeT;
    using weight_t = WeightT;

    Edge() : start(INVALID_ID), target(INVALID_ID), weight{} {}

    Edge(NodeT start_, NodeT target_, WeightT weight_)
        : start(start_), target(target_), weight(weight_) {}

    bool operator<(const Edge& rhs) const {
        return std::tie(start, target, weight) <
               std::tie(rhs.start, rhs.target, rhs.weight);
    }

    bool operator==(const Edge& rhs) const {
        return std::tie(start, target, weight) ==
               std::tie(rhs.start, rhs.target, rhs.weight);
    }

    bool operator!=(const Edge& rhs) const { return !operator==(rhs); }

    NodeT start;
    NodeT target;
    WeightT weight;
};

template <typename NodeT>
struct Edge<NodeT, void> {
    using node_t = NodeT;

    Edge() : start(INVALID_ID), target(INVALID_ID) {}

    Edge(NodeT start_, NodeT target_) : start(start_), target(target_) {}

    bool operator<(const Edge& rhs) const {
        return std::tie(start, target) < std::tie(rhs.start, rhs.target);
    }

    bool operator==(const Edge& rhs) const {
        return std::tie(start, target) == std::tie(rhs.start, rhs.target);
    }

    bool operator!=(const Edge& rhs) const { return !operator==(rhs); }

    NodeT start;
    NodeT target;
};
}
}

#endif
