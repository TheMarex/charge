#include "common/adj_graph.hpp"
#include "common/to_geojson.hpp"

#include <catch.hpp>

#include <vector>

using namespace charge;
using namespace charge::common;

using namespace Catch::Matchers;

TEST_CASE("Sprint simple graph", "[GeoJSON]") {
    // 0 -> 1
    // 0 -> 2
    // 1 -> 2
    AdjGraph graph{{0, 2, 3, 3}, {1, 2, 2}};

    std::vector<Coordinate> coordinates = {Coordinate::from_floating(0.5, 1.5),
                                           Coordinate::from_floating(2.5, 3.5),
                                           Coordinate::from_floating(-4.5, 5.500001)};

    auto result = graph_to_geojson(graph, coordinates);
    std::stringstream ss;
    ss << result;
    auto reference = "{\"features\":[{\"geometry\":{\"coordinates\":[[0.5,1.5],[2.5,3.5]],\"type\":"
                     "\"LineString\"},\"properties\":null,\"type\":\"Feature\"},{\"geometry\":{"
                     "\"coordinates\":[[0.5,1.5],[-4.5,5.500001]],\"type\":\"LineString\"},"
                     "\"properties\":null,\"type\":\"Feature\"},{\"geometry\":{\"coordinates\":[[2."
                     "5,3.5],[-4.5,5.500001]],\"type\":\"LineString\"},\"properties\":null,"
                     "\"type\":\"Feature\"}],\"type\":\"FeatureCollection\"}";

    REQUIRE(ss.str() == reference);
}
