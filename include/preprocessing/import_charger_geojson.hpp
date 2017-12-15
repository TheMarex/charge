#ifndef CHARGE_PREPROCESSING_IMPORT_GEOJSON_HPP
#define CHARGE_PREPROCESSING_IMPORT_GEOJSON_HPP

#include "common/nearest_neighbour.hpp"

#include <json.hpp>

namespace charge::preprocessing {

auto charger_from_geojson(const nlohmann::json &feature_collection,
                          const std::vector<common::Coordinate> &coordinates) {
    std::vector<double> chargers(coordinates.size(), 0.0);

    common::NearestNeighbour<> nn(coordinates);

    for (const auto &f : feature_collection["features"]) {
        common::Coordinate c = common::Coordinate::from_floating(f["geometry"]["coordinates"][0],
                                                                 f["geometry"]["coordinates"][1]);
        auto min = nn.nearest(c);
        auto dist = common::haversine_distance(coordinates[min], c);
        if (dist < 500) {
            chargers[min] = f["properties"]["rate"];
        }
    }

    return chargers;
}
} // namespace charge::preprocessing

#endif
