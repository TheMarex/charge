#include "common/nearest_neighbour.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Nearest with duplicate coordinates", "[nearest neighbor]") {
    // 5 +      5
    //   |
    // 4 +              2
    //   |
    // 3 +                               6
    //   |
    // 2 +       4               3
    //   |
    // 1 +   1           0
    //   |
    // 0 +---+---+---+---+---+---+---+---+
    //       1   2   3   4   5   6   7   8
    std::vector<Coordinate> coordinates{{
        Coordinate::from_floating(4, 1), Coordinate::from_floating(1, 1),
        Coordinate::from_floating(4, 4), Coordinate::from_floating(6, 2),
        Coordinate::from_floating(2, 2), Coordinate::from_floating(2, 5),
        Coordinate::from_floating(8, 3),
    }};

    NearestNeighbour<2> nn_2{coordinates};
    NearestNeighbour<3> nn_3{coordinates};
    NearestNeighbour<4> nn_4{coordinates};

    auto reference = [&](const auto &coordinate) -> std::size_t {
        auto iter = std::min_element(coordinates.begin(), coordinates.end(),
                                     [&](const auto &lhs, const auto &rhs) {
                                         return euclid_squared_distance(coordinate, lhs) <
                                                euclid_squared_distance(coordinate, rhs);
                                     });
        return std::distance(coordinates.begin(), iter);
    };

    auto query_1 = Coordinate::from_floating(5, 5);
    auto query_2 = Coordinate::from_floating(7.5, 3);
    auto query_3 = Coordinate::from_floating(2, 3);
    auto query_4 = Coordinate::from_floating(2, 5);
    auto query_5 = Coordinate::from_floating(1, 1);
    auto query_6 = Coordinate::from_floating(4, 0);

    CHECK(nn_2.nearest(query_1) == reference(query_1));
    CHECK(nn_2.nearest(query_2) == reference(query_2));
    CHECK(nn_2.nearest(query_3) == reference(query_3));
    CHECK(nn_2.nearest(query_4) == reference(query_4));
    CHECK(nn_2.nearest(query_5) == reference(query_5));
    CHECK(nn_2.nearest(query_6) == reference(query_6));

    CHECK(nn_3.nearest(query_1) == reference(query_1));
    CHECK(nn_3.nearest(query_2) == reference(query_2));
    CHECK(nn_3.nearest(query_3) == reference(query_3));
    CHECK(nn_3.nearest(query_4) == reference(query_4));
    CHECK(nn_3.nearest(query_5) == reference(query_5));
    CHECK(nn_3.nearest(query_6) == reference(query_6));

    CHECK(nn_4.nearest(query_1) == reference(query_1));
    CHECK(nn_4.nearest(query_2) == reference(query_2));
    CHECK(nn_4.nearest(query_3) == reference(query_3));
    CHECK(nn_4.nearest(query_4) == reference(query_4));
    CHECK(nn_4.nearest(query_5) == reference(query_5));
    CHECK(nn_4.nearest(query_6) == reference(query_6));
}
