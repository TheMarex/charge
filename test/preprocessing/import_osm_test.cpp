#include "preprocessing/import_osm.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::preprocessing;

namespace Catch
{
    template<>
    std::string toString<charge::preprocessing::OSMNetwork::NodeMetaData>(const charge::preprocessing::OSMNetwork::NodeMetaData& n)
    {
        std::stringstream ss;
        ss << "{" << n.height << "," << n.coordinate << "}";
        return ss.str();
    }

    template<>
    std::string toString<charge::preprocessing::OSMNetwork::EdgeMetaData>(const charge::preprocessing::OSMNetwork::EdgeMetaData& e)
    {
        std::stringstream ss;
        ss << "{" << e.min_speed << "," << e.max_speed << "}";
        return ss.str();
    }
}

TEST_CASE("Simplify OSM data", "[osm]") {
    // Can remove 1 and 3.
    //
    // 3 -> 4 -> 5 or 2 -> 4 -> 5 is not compatible.
    //
    // 7 -> 6 -> 2 is not downhill/uphill
    //
    // 0 -> 1 -> 2 -> 3 -> 4 -> 5
    //   <-   <- ^
    //           |
    //           6
    //           ^
    //           |
    //           7

    OSMNetwork network{{
                           {0, 1, {13, 26, false}},
                           {1, 0, {26, 72, false}},
                           {1, 2, {13, 26, false}},
                           {2, 1, {26, 72, false}},
                           {2, 3, {13, 13, false}},
                           {3, 4, {13, 13, false}},
                           {4, 5, {13, 26, false}},
                           {6, 2, {13, 26, false}},
                           {7, 6, {13, 13, false}},
                       },
                       {{0, common::Coordinate::from_floating(0.0, 0.0)},
                        {1, common::Coordinate::from_floating(1.0, 0.0)},
                        {3, common::Coordinate::from_floating(2.0, 0.0)},
                        {2, common::Coordinate::from_floating(3.0, 0.0)},
                        {1, common::Coordinate::from_floating(4.0, 0.0)},
                        {0, common::Coordinate::from_floating(5.0, 0.0)},
                        {0, common::Coordinate::from_floating(2.0, -1.0)},
                        {1, common::Coordinate::from_floating(2.0, -2.0)}}};

    OSMNetwork reference{{
                           {0, 1, {13, 26, false}},
                           {1, 0, {26, 72, false}},
                           {1, 2, {13, 13, false}},
                           {2, 3, {13, 26, false}},
                           {4, 1, {13, 26, false}},
                           {5, 4, {13, 13, false}},
                       },
                       {{0, common::Coordinate::from_floating(0.0, 0.0)},
                        {3, common::Coordinate::from_floating(2.0, 0.0)},
                        {1, common::Coordinate::from_floating(4.0, 0.0)},
                        {0, common::Coordinate::from_floating(5.0, 0.0)},
                        {0, common::Coordinate::from_floating(2.0, -1.0)},
                        {1, common::Coordinate::from_floating(2.0, -2.0)}}};

    auto result = simplify_network(network, 0);

    CHECK(result.edges == reference.edges);
    CHECK(result.nodes == reference.nodes);
}

TEST_CASE("Remove small component from OSM data", "[osm]") {
    // Can remove 1, 3, 4, 5.
    //
    // 7 -> 6 -> 2 is not downhill/uphill
    //
    // 0 -> 1 -> 2       3 -> 4 -> 5
    //   <-   <- ^
    //           |
    //           6
    //           ^
    //           |
    //           7

    OSMNetwork network{{
                           {0, 1, {13, 26, false}},
                           {1, 0, {26, 72, false}},
                           {1, 2, {13, 26, false}},
                           {2, 1, {26, 72, false}},
                           {3, 4, {13, 13, false}},
                           {4, 5, {13, 26, false}},
                           {6, 2, {13, 26, false}},
                           {7, 6, {13, 13, false}},
                       },
                       {{0, common::Coordinate::from_floating(0.0, 0.0)},
                        {1, common::Coordinate::from_floating(1.0, 0.0)},
                        {3, common::Coordinate::from_floating(2.0, 0.0)},
                        {2, common::Coordinate::from_floating(3.0, 0.0)},
                        {1, common::Coordinate::from_floating(4.0, 0.0)},
                        {0, common::Coordinate::from_floating(5.0, 0.0)},
                        {0, common::Coordinate::from_floating(2.0, -1.0)},
                        {1, common::Coordinate::from_floating(2.0, -2.0)}}};

    OSMNetwork reference{{
                           {0, 1, {13, 26, false}},
                           {1, 0, {26, 72, false}},
                           {2, 1, {13, 26, false}},
                           {3, 2, {13, 13, false}},
                       },
                       {{0, common::Coordinate::from_floating(0.0, 0.0)},
                        {3, common::Coordinate::from_floating(2.0, 0.0)},
                        {0, common::Coordinate::from_floating(2.0, -1.0)},
                        {1, common::Coordinate::from_floating(2.0, -2.0)}}};


    auto result = simplify_network(network, 4);

    CHECK(result.edges == reference.edges);
    CHECK(result.nodes == reference.nodes);
}
