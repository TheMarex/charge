#include "common/path.hpp"
#include "common/adj_graph.hpp"

#include <catch.hpp>

#include <vector>

#include <iostream>

using namespace charge;
using namespace charge::common;

using namespace Catch::Matchers;

TEST_CASE("Test unpacking path from two vectors", "[Path]") {
    // Path to be unpacked:
    //       1                    7
    //       ^                    ^
    //       |                    |
    // 0---->2-->4---->3--->5---->8--->9
    // start        middle           target
    ParentVector<AdjGraph> forward_parents(10, INVALID_ID);
    ParentVector<AdjGraph> reverse_parents(10, INVALID_ID);

    forward_parents[0] = INVALID_ID;
    forward_parents[1] = 2;
    forward_parents[2] = 0;
    forward_parents[3] = 4;
    forward_parents[4] = 2;
    forward_parents[5] = INVALID_ID;
    forward_parents[6] = INVALID_ID;
    forward_parents[7] = INVALID_ID;
    forward_parents[8] = INVALID_ID;
    forward_parents[9] = INVALID_ID;

    reverse_parents[0] = INVALID_ID;
    reverse_parents[1] = INVALID_ID;
    reverse_parents[2] = INVALID_ID;
    reverse_parents[3] = 5;
    reverse_parents[4] = INVALID_ID;
    reverse_parents[5] = 8;
    reverse_parents[6] = INVALID_ID;
    reverse_parents[7] = 8;
    reverse_parents[8] = 9;
    reverse_parents[9] = INVALID_ID;

    auto path = get_path<AdjGraph>(0, 3, 9, forward_parents, reverse_parents);

    std::vector<AdjGraph::node_id_t> reference{0, 2, 4, 3, 5, 8, 9};

    REQUIRE(path == reference);
}

TEST_CASE("Test computing cost from a path", "[Path]") {
    // Path to be unpacked:
    //       1                    7
    //       ^                    ^
    //       |                    |
    // 0---->2-->4---->3--->5---->8--->9
    // start        middle           target
    ParentVector<AdjGraph> forward_parents(10, INVALID_ID);
    ParentVector<AdjGraph> reverse_parents(10, INVALID_ID);

    forward_parents[0] = INVALID_ID;
    forward_parents[1] = 2;
    forward_parents[2] = 0;
    forward_parents[3] = 4;
    forward_parents[4] = 2;
    forward_parents[5] = INVALID_ID;
    forward_parents[6] = INVALID_ID;
    forward_parents[7] = INVALID_ID;
    forward_parents[8] = INVALID_ID;
    forward_parents[9] = INVALID_ID;

    reverse_parents[0] = INVALID_ID;
    reverse_parents[1] = INVALID_ID;
    reverse_parents[2] = INVALID_ID;
    reverse_parents[3] = 5;
    reverse_parents[4] = INVALID_ID;
    reverse_parents[5] = 8;
    reverse_parents[6] = INVALID_ID;
    reverse_parents[7] = 8;
    reverse_parents[8] = 9;
    reverse_parents[9] = INVALID_ID;

    CostVector<WeightedGraph<std::int32_t>> forward_costs(10, INF_WEIGHT);
    CostVector<WeightedGraph<std::int32_t>> reverse_costs(10, INF_WEIGHT);

    forward_costs[0] = 0;
    forward_costs[1] = 2;
    forward_costs[2] = 1;
    forward_costs[3] = 5;
    forward_costs[4] = 3;
    forward_costs[5] = INF_WEIGHT;
    forward_costs[6] = INF_WEIGHT;
    forward_costs[7] = INF_WEIGHT;
    forward_costs[8] = INF_WEIGHT;
    forward_costs[9] = INF_WEIGHT;

    reverse_costs[0] = INF_WEIGHT;
    reverse_costs[1] = INF_WEIGHT;
    reverse_costs[2] = INF_WEIGHT;
    reverse_costs[3] = 5;
    reverse_costs[4] = INF_WEIGHT;
    reverse_costs[5] = 4;
    reverse_costs[6] = INF_WEIGHT;
    reverse_costs[7] = 3;
    reverse_costs[8] = 2;
    reverse_costs[9] = 0;

    auto[path, path_labels] = get_path_with_labels<WeightedGraph<std::int32_t>>(
        0, 3, 9, forward_parents, reverse_parents, forward_costs, reverse_costs);
    std::vector<std::int32_t> reference{0, 1, 3, 5, 6, 8, 10};
    REQUIRE(path_labels == reference);
}
