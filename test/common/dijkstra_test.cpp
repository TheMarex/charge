#include "common/dijkstra.hpp"

#include "common/graph_transform.hpp"
#include "common/id_queue.hpp"
#include "common/lazy_clear_vector.hpp"
#include "common/path.hpp"
#include "common/weighted_graph.hpp"

#include <catch.hpp>

#include <vector>

using namespace charge;
using namespace charge::common;

namespace {
using TestGraph = WeightedGraph<std::int32_t>;
using TestCostVector = CostVector<TestGraph>;
using TestParentVector = ParentVector<TestGraph>;
}

TEST_CASE("Stall at nodes", "[dijkstra]") {
    // 0 -> 1 -> 2
    std::vector<TestGraph::edge_t> edges{{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
    TestGraph forward_graph{4, edges};
    TestGraph reverse_graph = invert(forward_graph);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    const auto stall_at_node_1 = [](const TestGraph::node_id_t key) { return key == 1; };

    REQUIRE(3 == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>,
                          no_stall<TestGraph>));
    REQUIRE(INF_WEIGHT == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                                   forward_costs, reverse_costs, terminate_sum_min<TestGraph>,
                                   stall_at_node_1));
}

TEST_CASE("Test oneways", "[dijkstra]") {
    // 0 -> 1 -> 2 -> 3
    //      ^---------|
    std::vector<TestGraph::edge_t> edges{{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {3, 1, 1}};
    TestGraph forward_graph{4, edges};
    TestGraph reverse_graph = invert(forward_graph);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    REQUIRE(3 == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs));
    REQUIRE(INF_WEIGHT == dijkstra(3, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                                   forward_costs, reverse_costs));

    REQUIRE(2 == dijkstra(1, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs));
    REQUIRE(1 == dijkstra(3, 1, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs));
}

TEST_CASE("Test oneways in CH", "[dijkstra]") {
    // 2       2
    //        / \
    // 1     1   3
    //      /     \
    // 0   0       4
    std::vector<TestGraph::edge_t> edges{{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {3, 4, 1}};
    std::vector<unsigned> rank = {0, 1, 2, 1, 0};
    TestGraph forward_graph{5, edges};
    TestGraph reverse_graph = invert(forward_graph);
    forward_graph = filterByRank(forward_graph, rank);
    reverse_graph = filterByRank(reverse_graph, rank);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    REQUIRE(4 == dijkstra(0, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(INF_WEIGHT == dijkstra(4, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                                   forward_costs, reverse_costs,
                                   terminate_queues_empty<TestGraph>));
}

TEST_CASE("Test full graph", "[dijkstra]") {
    // 0 -- 1 -- 2
    // |    |    |
    // 3 -- 4 -- 5
    // |    |    |
    // 6 -- 7 -- 8
    std::vector<TestGraph::edge_t> edges{
        {0, 1, 1}, {0, 3, 5}, {1, 0, 5}, {1, 2, 1}, {1, 4, 1}, {2, 1, 5}, {2, 5, 1}, {3, 0, 1},
        {3, 4, 4}, {3, 6, 5}, {4, 1, 1}, {4, 3, 1}, {4, 5, 3}, {4, 7, 1}, {5, 2, 5}, {5, 4, 10},
        {5, 8, 2}, {6, 3, 1}, {6, 7, 5}, {7, 4, 1}, {7, 6, 1}, {7, 8, 5}, {8, 5, 5}, {8, 7, 1}};
    TestGraph forward_graph{9, edges};
    TestGraph reverse_graph = invert(forward_graph);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    // 3, 0, 1, 2, 5
    REQUIRE(4 == dijkstra(3, 5, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>));
    REQUIRE(4 == dijkstra(3, 5, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    // 5, 8, 7, 6, 3
    REQUIRE(5 == dijkstra(5, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>));
    REQUIRE(5 == dijkstra(5, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    // 3, 0, 1, 4
    REQUIRE(3 == dijkstra(3, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>));
    REQUIRE(3 == dijkstra(3, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    // 4, 3
    REQUIRE(1 == dijkstra(4, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>));
    REQUIRE(1 == dijkstra(4, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
}

TEST_CASE("Test CH Graph", "[dijkstra]") {
    // 2               4
    //               /
    // 1     1     3
    //     /   \  /
    // 0  0     2
    std::vector<TestGraph::edge_t> edges{{0, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 3, 2}, // shortcut
                                         {1, 4, 3},                                  // shortcut
                                         {2, 1, 1}, {2, 3, 1}, {3, 1, 2},            // shortcut
                                         {3, 2, 1}, {3, 4, 1}, {4, 1, 3},            // shortcut
                                         {4, 3, 1}};
    std::vector<unsigned> rank = {0, 1, 0, 1, 2};

    TestGraph forward_graph{5, edges};
    TestGraph reverse_graph = invert(forward_graph);
    forward_graph = filterByRank(forward_graph, rank);
    reverse_graph = filterByRank(reverse_graph, rank);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    REQUIRE(1 == dijkstra(0, 1, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(1 == dijkstra(1, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(2 == dijkstra(0, 2, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(2 == dijkstra(2, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(3 == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(3 == dijkstra(3, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(4 == dijkstra(0, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(4 == dijkstra(4, 0, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(1 == dijkstra(1, 2, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(1 == dijkstra(2, 1, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(2 == dijkstra(1, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(2 == dijkstra(3, 1, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(3 == dijkstra(1, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(3 == dijkstra(4, 1, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(1 == dijkstra(2, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(1 == dijkstra(3, 2, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(2 == dijkstra(2, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(2 == dijkstra(4, 2, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));

    REQUIRE(1 == dijkstra(3, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(1 == dijkstra(4, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
}

TEST_CASE("Termination", "[dijkstra]") {
    // 6               6
    //                /
    // 5             5
    //              /
    // 4           4
    //            /
    // 3         3
    //          / |
    // 2       2  |
    //       /    |
    // 1    1     |
    //     /      |
    // 0  0-------|
    std::vector<TestGraph::edge_t> edges{{0, 1, 1},  {0, 3, 4},  {1, 0, 1},  {1, 2, 1},  {2, 1, 1},
                                         {2, 3, 1},  {3, 2, 1},  {3, 4, 10}, {4, 3, 10}, {4, 5, 10},
                                         {5, 4, 10}, {5, 6, 10}, {6, 5, 10}};
    std::vector<unsigned> rank = {0, 1, 2, 3, 4, 5, 6};

    TestGraph forward_graph{7, edges};
    TestGraph reverse_graph = invert(forward_graph);
    forward_graph = filterByRank(forward_graph, rank);
    reverse_graph = filterByRank(reverse_graph, rank);
    assert(reverse_graph.num_nodes() == forward_graph.num_nodes());

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    TestCostVector forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    TestCostVector reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);

    REQUIRE(3 == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_queues_empty<TestGraph>));
    REQUIRE(4 == dijkstra(0, 3, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, terminate_sum_min<TestGraph>));
}

TEST_CASE("Reference graph searches", "[dijkstra]") {
    using Graph = common::WeightedGraph<std::int32_t>;
    using node_id_t = Graph::node_id_t;

    //                        8
    //                        |
    //   0-----1-----2----4---7---9
    //    \   /      |    |   |
    //     \ /       |    |   10
    //      3        5----6
    Graph forward_graph{11,
                        std::vector<common::WeightedGraph<std::int32_t>::edge_t>{
                            {0, 1, 1}, {0, 3, 1}, {1, 0, 2},  {1, 2, 2}, {1, 3, 2},  {2, 1, 3},
                            {2, 4, 3}, {2, 5, 3}, {3, 0, 4},  {3, 1, 4}, {4, 2, 5},  {4, 6, 5},
                            {4, 7, 5}, {5, 2, 6}, {5, 6, 20}, {6, 4, 7}, {6, 5, 7},  {7, 4, 8},
                            {7, 8, 8}, {7, 9, 8}, {7, 10, 8}, {8, 7, 9}, {9, 7, 10}, {10, 7, 11}}};

    auto reverse_graph = invert(forward_graph);

    MinIDQueue forward_queue(forward_graph.num_nodes());
    MinIDQueue reverse_queue(reverse_graph.num_nodes());
    CostVector<Graph> forward_costs(forward_graph.num_nodes(), INF_WEIGHT);
    CostVector<Graph> reverse_costs(reverse_graph.num_nodes(), INF_WEIGHT);
    ParentVector<Graph> forward_parents(forward_graph.num_nodes(), INVALID_ID);
    ParentVector<Graph> reverse_parents(reverse_graph.num_nodes(), INVALID_ID);

    node_id_t middle = INVALID_ID;

    // 0 -> 9:  0-1-2-4-7-9 (1,2,3,5,8)
    // 5 -> 6:  5-2-4-6     (6,3,5)
    // 3 -> 8:  3-1-2-4-7-8 (4,2,3,5,8)
    // 3 -> 6:  3-1-2-4-6   (4,2,3,5)
    // 9 -> 5:  9-7-4-2-5   (10,8,5,3)
    // 10 -> 9: 10-7-9      (11,8)
    // 2 -> 4:  2-4         (3)
    // 2 -> 2:  2           ()
    REQUIRE(19 == dijkstra(0, 9, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_1 = {0, 1, 2, 4, 7, 9};
    std::vector<Graph::weight_t> reference_costs_1 = {0, 1, 3, 6, 11, 19};
    const auto[path_1, costs_1] = common::get_path_with_labels<Graph>(
        0, middle, 9, forward_parents, reverse_parents, forward_costs, reverse_costs);
    REQUIRE(reference_path_1 == path_1);
    REQUIRE(reference_costs_1 == costs_1);

    REQUIRE(14 == dijkstra(5, 6, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_2 = {5, 2, 4, 6};
    REQUIRE(reference_path_2 ==
            common::get_path<Graph>(5, middle, 6, forward_parents, reverse_parents));

    REQUIRE(22 == dijkstra(3, 8, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_3 = {3, 1, 2, 4, 7, 8};
    REQUIRE(reference_path_3 ==
            common::get_path<Graph>(3, middle, 8, forward_parents, reverse_parents));

    REQUIRE(14 == dijkstra(3, 6, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_4 = {3, 1, 2, 4, 6};
    REQUIRE(reference_path_4 ==
            common::get_path<Graph>(3, middle, 6, forward_parents, reverse_parents));

    REQUIRE(26 == dijkstra(9, 5, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_5 = {9, 7, 4, 2, 5};
    REQUIRE(reference_path_5 ==
            common::get_path<Graph>(9, middle, 5, forward_parents, reverse_parents));

    REQUIRE(19 == dijkstra(10, 9, forward_graph, reverse_graph, forward_queue, reverse_queue,
                           forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_6 = {10, 7, 9};
    REQUIRE(reference_path_6 ==
            common::get_path<Graph>(10, middle, 9, forward_parents, reverse_parents));

    REQUIRE(3 == dijkstra(2, 4, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_7 = {2, 4};
    REQUIRE(reference_path_7 ==
            common::get_path<Graph>(2, middle, 4, forward_parents, reverse_parents));

    REQUIRE(0 == dijkstra(2, 2, forward_graph, reverse_graph, forward_queue, reverse_queue,
                          forward_costs, reverse_costs, forward_parents, reverse_parents, middle));
    std::vector<node_id_t> reference_path_8 = {2};
    REQUIRE(reference_path_8 ==
            common::get_path<Graph>(2, middle, 2, forward_parents, reverse_parents));
}

TEST_CASE("Reference graph searches with uni-directional", "[dijkstra]") {
    using Graph = common::WeightedGraph<std::int32_t>;
    using node_id_t = Graph::node_id_t;

    //                        8
    //                        |
    //   0-----1-----2----4---7---9
    //    \   /      |    |   |
    //     \ /       |    |   10
    //      3        5----6
    Graph graph{11, std::vector<common::WeightedGraph<std::int32_t>::edge_t>{
                        {0, 1, 1}, {0, 3, 1}, {1, 0, 2},  {1, 2, 2}, {1, 3, 2},  {2, 1, 3},
                        {2, 4, 3}, {2, 5, 3}, {3, 0, 4},  {3, 1, 4}, {4, 2, 5},  {4, 6, 5},
                        {4, 7, 5}, {5, 2, 6}, {5, 6, 20}, {6, 4, 7}, {6, 5, 7},  {7, 4, 8},
                        {7, 8, 8}, {7, 9, 8}, {7, 10, 8}, {8, 7, 9}, {9, 7, 10}, {10, 7, 11}}};

    MinIDQueue queue(graph.num_nodes());
    CostVector<Graph> costs(graph.num_nodes(), INF_WEIGHT);
    ParentVector<Graph> parents(graph.num_nodes(), INVALID_ID);

    node_id_t middle = INVALID_ID;

    // 0 -> 9:  0-1-2-4-7-9 (1,2,3,5,8)
    // 5 -> 6:  5-2-4-6     (6,3,5)
    // 3 -> 8:  3-1-2-4-7-8 (4,2,3,5,8)
    // 3 -> 6:  3-1-2-4-6   (4,2,3,5)
    // 9 -> 5:  9-7-4-2-5   (10,8,5,3)
    // 10 -> 9: 10-7-9      (11,8)
    // 2 -> 4:  2-4         (3)
    // 2 -> 2:  2           ()
    REQUIRE(19 == dijkstra(0, 9, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_1 = {0, 1, 2, 4, 7, 9};
    std::vector<Graph::weight_t> reference_costs_1 = {0, 1, 3, 6, 11, 19};
    const auto[path_1, costs_1] = common::get_path_with_labels<Graph>(0, 9, parents, costs);
    REQUIRE(reference_path_1 == path_1);
    REQUIRE(reference_costs_1 == costs_1);

    REQUIRE(14 == dijkstra(5, 6, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_2 = {5, 2, 4, 6};
    const auto[path_2, costs_2] = common::get_path_with_labels<Graph>(5, 6, parents, costs);
    REQUIRE(reference_path_2 == path_2);

    REQUIRE(22 == dijkstra(3, 8, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_3 = {3, 1, 2, 4, 7, 8};
    const auto[path_3, costs_3] = common::get_path_with_labels<Graph>(3, 8, parents, costs);
    REQUIRE(reference_path_3 == path_3);

    REQUIRE(14 == dijkstra(3, 6, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_4 = {3, 1, 2, 4, 6};
    const auto[path_4, costs_4] = common::get_path_with_labels<Graph>(3, 6, parents, costs);
    REQUIRE(reference_path_4 == path_4);

    REQUIRE(26 == dijkstra(9, 5, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_5 = {9, 7, 4, 2, 5};
    const auto[path_5, costs_5] = common::get_path_with_labels<Graph>(9, 5, parents, costs);
    REQUIRE(reference_path_5 == path_5);

    REQUIRE(19 == dijkstra(10, 9, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_6 = {10, 7, 9};
    const auto[path_6, costs_6] = common::get_path_with_labels<Graph>(10, 9, parents, costs);
    REQUIRE(reference_path_6 == path_6);

    REQUIRE(3 == dijkstra(2, 4, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_7 = {2, 4};
    const auto[path_7, costs_7] = common::get_path_with_labels<Graph>(2, 4, parents, costs);
    REQUIRE(reference_path_7 == path_7);

    REQUIRE(0 == dijkstra(2, 2, graph, queue, costs, parents));
    std::vector<node_id_t> reference_path_8 = {2};
    const auto[path_8, costs_8] = common::get_path_with_labels<Graph>(2, 2, parents, costs);
    REQUIRE(reference_path_8 == path_8);
}
