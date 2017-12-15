#ifndef CHARGE_COMMON_SEARCH_SPACE_HPP
#define CHARGE_COMMON_SEARCH_SPACE_HPP

#include "common/coordinate.hpp"
#include "common/irange.hpp"

#include <vector>

namespace charge::common {

struct SearchSpaceNode {
    common::Coordinate coordinate;
    std::size_t id;
    std::size_t num_settled_labels;
    bool is_charging_station;
};

template <typename NodeLabelsT, typename ChargingFunctionContainerT>
inline auto get_search_space(const NodeLabelsT &labels, const ChargingFunctionContainerT &chargers,
                             const std::vector<Coordinate> &coordinates) {
    auto num_nodes = labels.settled_labels.size();
    std::vector<SearchSpaceNode> search_space;

    for (const auto node : common::irange<std::size_t>(0, num_nodes)) {
        if (!labels[node].empty()) {
            search_space.push_back(SearchSpaceNode{coordinates[node], node, labels[node].size(),
                                                   chargers.weighted(node)});
        }
    }

    return search_space;
}

template <typename NodeLabelsT>
inline auto get_search_space(const NodeLabelsT &labels,
                             const std::vector<Coordinate> &coordinates) {
    auto num_nodes = labels.settled_labels.size();
    std::vector<SearchSpaceNode> search_space;

    for (const auto node : common::irange<std::size_t>(0, num_nodes)) {
        if (!labels[node].empty()) {
            search_space.push_back(
                SearchSpaceNode{coordinates[node], node, labels[node].size(), false});
        }
    }

    return search_space;
}

} // namespace charge::common

#endif
