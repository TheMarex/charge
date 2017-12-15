#include "common/adj_graph.hpp"

#include <catch.hpp>

#include <vector>

using namespace charge;
using namespace charge::common;

using namespace Catch::Matchers;

TEST_CASE("Test construction and iterators", "[AdjGraph]")
{
    // 0 -> 1
    // 0 -> 2
    // 1 -> 2
    AdjGraph reference_1 {
        {0, 2, 3, 3},
        {1, 2, 2}
    };
    REQUIRE(reference_1.begin(0) == 0);
    REQUIRE(reference_1.end(0) == 2);
    REQUIRE(reference_1.begin(1) == 2);
    REQUIRE(reference_1.end(1) == 3);
    REQUIRE(reference_1.begin(2) == 3);
    REQUIRE(reference_1.end(2) == 3);
    REQUIRE(reference_1.target(0) == 1);
    REQUIRE(reference_1.target(1) == 2);
    REQUIRE(reference_1.target(2) == 2);

    // 1 -> 0
    // 2 -> 0
    // 2 -> 1
    AdjGraph reference_2 {
        {0, 0, 1, 3},
        {0, 0, 1}
    };

    std::vector<AdjGraph::edge_t> input_edges { {1, 0}, {2, 0}, {2, 1} };
    auto output = AdjGraph {3, input_edges};
    REQUIRE_THAT(output.edges(), Equals(reference_2.edges()));
}

