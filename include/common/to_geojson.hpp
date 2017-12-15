#ifndef CHARGE_COMMON_TO_GEOJSON_HPP
#define CHARGE_COMMON_TO_GEOJSON_HPP

#include "common/coordinate.hpp"
#include "common/irange.hpp"
#include "common/search_space.hpp"

#include <json.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace charge {
namespace common {

using json = nlohmann::json;

namespace detail {
template <typename EdgeT>
auto edge_to_feature(const EdgeT &, const common::Coordinate &start_coord,
                     const common::Coordinate &target_coord, json properties = {}) {
    double lon_0, lon_1, lat_0, lat_1;
    std::tie(lon_0, lat_0) = start_coord.to_floating();
    std::tie(lon_1, lat_1) = target_coord.to_floating();

    json geometry;
    geometry["type"] = "LineString";
    geometry["coordinates"] = std::vector<std::vector<double>>{{{lon_0, lat_0}}, {lon_1, lat_1}};

    json feature;
    feature["type"] = "Feature";
    feature["properties"] = properties;
    feature["geometry"] = geometry;
    return feature;
}

template <typename NodeT>
auto node_to_feature(const NodeT &, const Coordinate &coordinate, const json &properties = {}) {
    double lon_0, lat_0;
    std::tie(lon_0, lat_0) = coordinate.to_floating();

    json geometry;
    geometry["type"] = "Point";
    geometry["coordinates"] = std::vector<double>{{lon_0, lat_0}};

    json feature;
    feature["type"] = "Feature";
    feature["properties"] = properties;
    feature["geometry"] = geometry;
    return feature;
}

auto features_to_featurecollection(const json &features) {
    json fc;
    fc["type"] = "FeatureCollection";
    fc["features"] = features;
    return fc;
}
} // namespace detail

inline json chargers_to_geojson(const std::vector<double> &chargers,
                                const std::vector<common::Coordinate> &coordinates) {
    json features = json::array();
    for (auto index : common::irange<std::size_t>(0, chargers.size())) {
        if (chargers[index] > 0)
            features.push_back(common::detail::node_to_feature(
                index, coordinates[index], {{"rate", std::to_string(chargers[index])}}));
    }
    return common::detail::features_to_featurecollection(features);
}

template <typename GraphT>
json graph_to_geojson(const GraphT &graph, const std::vector<Coordinate> &coordinates) {
    json features = json::array();
    for (const auto &edge : graph.edges())
        features.push_back(
            detail::edge_to_feature(edge, coordinates[edge.start], coordinates[edge.target]));

    return detail::features_to_featurecollection(features);
}

inline json search_space_to_geojson(const std::vector<SearchSpaceNode> &search_space) {
    json features = json::array();
    for (const auto &node : search_space) {
        features.push_back(common::detail::node_to_feature(
            node.id, node.coordinate,
            {{"labels", std::to_string(node.num_settled_labels)},
             {"charger", node.is_charging_station ? "true" : "false"}}));
    }
    return common::detail::features_to_featurecollection(features);
}

} // namespace common
} // namespace charge

#endif
