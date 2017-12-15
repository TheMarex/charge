#include "common/fp_dijkstra.hpp"

#include "common/id_queue.hpp"
#include "common/path.hpp"

#include "ev/graph.hpp"
#include "ev/fp_dijkstra.hpp"

#include "../helper/function_printer.hpp"
#include "../helper/label_printer.hpp"

#include <catch.hpp>

#include <vector>

using namespace charge;
using namespace charge::common;

namespace {
using TestGraph = common::HypLinGraph;
using TestLabelEntry = LabelEntry<TestGraph::weight_t, TestGraph::node_id_t>;
using TestLabelEntryWithParent = LabelEntryWithParent<TestGraph::weight_t, TestGraph::node_id_t>;
using TestLabels = NodeLabels<common::FPDijkstraPolicy<TestGraph, TestLabelEntry>>;
using TestLabelsWithParents = NodeLabels<common::FPDijkstraPolicy<TestGraph, TestLabelEntryWithParent>>;
}

TEST_CASE("Test oneways with FP", "[mc dijkstra]") {
    // 0 -> 1 -> 2 -> 3
    //      ^---------|
    std::vector<TestGraph::edge_t> edges{
        {0, 1, ev::make_constant(1, 2)}, {1, 2, ev::make_constant(1, 3)}, {2, 3, ev::make_constant(1, 4)}, {3, 1, ev::make_constant(1, 2)}};
    TestGraph graph{4, edges};

    MinIDQueue queue(graph.num_nodes());
    TestLabels labels(graph.num_nodes());

    auto results_1 = fp_dijkstra(0, 3, graph, queue, labels);
    std::vector<TestLabelEntry> reference_1{{3, ev::make_constant(3, 9)}};
    REQUIRE(results_1 == reference_1);

    auto results_2 = fp_dijkstra(3, 0, graph, queue, labels);
    std::vector<TestLabelEntry> reference_2{};
    REQUIRE(results_2 == reference_2);

    TestLabelsWithParents labels_with_parents(graph.num_nodes());
    auto results_3 = fp_dijkstra(0, 3, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_1{0, 1, 2, 3};
    REQUIRE(reference_path_1 ==
            common::get_path(0, 3, results_3.front(), labels_with_parents));
}

TEST_CASE("Test full graph with FP", "[fp dijkstra]") {
    // 0 --- 1 --- 2
    // |  \  |     |
    // 3 --- 4 --- 5
    // |     |  \  |
    // 6 --- 7 --- 8
    std::vector<TestGraph::edge_t> edges{{0, 1, ev::make_constant(1, 1)},  //
                                         {0, 3, ev::make_constant(1, 1)},  //
                                         {0, 4, ev::make_constant(5, 1)},  //
                                         {1, 0, ev::make_constant(1, 1)},  //
                                         {1, 2, ev::make_constant(1, 1)},  //
                                         {1, 4, ev::make_constant(1, 1)},  //
                                         {2, 1, ev::make_constant(1, 1)},  //
                                         {2, 5, ev::make_constant(1, 1)},  //
                                         {3, 0, ev::make_constant(1, 1)},  //
                                         {3, 4, ev::make_constant(1, 1)},  //
                                         {3, 6, ev::make_constant(1, 1)},  //
                                         {4, 1, ev::make_constant(1, 1)},  //
                                         {4, 3, ev::make_constant(1, 1)},  //
                                         {4, 5, ev::make_constant(1, 1)},  //
                                         {4, 7, ev::make_constant(1, 1)},  //
                                         {4, 8, ev::make_constant(5, 1)},  //
                                         {5, 2, ev::make_constant(1, 1)},  //
                                         {5, 4, ev::make_constant(1, 1)},  //
                                         {5, 8, ev::make_constant(1, 1)},  //
                                         {6, 3, ev::make_constant(1, 1)},  //
                                         {6, 7, ev::make_constant(1, 1)},  //
                                         {7, 4, ev::make_constant(1, 1)},  //
                                         {7, 6, ev::make_constant(1, 1)},  //
                                         {7, 8, ev::make_constant(1, 1)},  //
                                         {8, 5, ev::make_constant(1, 1)},  //
                                         {8, 7, ev::make_constant(1, 1)}};
    TestGraph graph{9, edges};

    MinIDQueue queue(graph.num_nodes());
    TestLabels labels(graph.num_nodes());

    auto results_1 = fp_dijkstra(0, 8, graph, queue, labels);
    // first solution is all paths using 4 edges (only horizontal and vertial)
    // second solution is all paths using 3 edges (one diagonal, two horizontal
    // or vertial)
    // third solution is all paths using 2 edges (two diagonal)
    std::vector<TestLabelEntry> reference_1{{4, ev::make_constant(4, 4)}, {7, ev::make_constant(7, 3)}, {10, ev::make_constant(10, 2)}};
    REQUIRE(results_1 == reference_1);

    auto results_2 = fp_dijkstra(0, 6, graph, queue, labels);
    // only one solution, using diagonals does not help
    std::vector<TestLabelEntry> reference_2{{2, ev::make_constant(2, 2)}};
    REQUIRE(results_1 == reference_1);

    auto results_3 = fp_dijkstra(0, 5, graph, queue, labels);
    // first solution only uses horizontal or vertical edges
    // second solution uses one diagonal and one horizontal
    std::vector<TestLabelEntry> reference_3{{3, ev::make_constant(3, 3)}, {6, ev::make_constant(6, 2)}};
    REQUIRE(results_1 == reference_1);

    TestLabelsWithParents labels_with_parents(graph.num_nodes());

    auto results_4 = fp_dijkstra(0, 8, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_1{0, 1, 2, 5, 8};
    std::vector<TestGraph::node_id_t> reference_path_2{0, 1, 4, 8};
    std::vector<TestGraph::node_id_t> reference_path_3{0, 4, 8};
    REQUIRE(reference_path_1 == common::get_path(0, 8, results_4.front(), labels_with_parents));
    REQUIRE(reference_path_2 == common::get_path(0, 8, results_4[1], labels_with_parents));
    REQUIRE(reference_path_3 == common::get_path(0, 8, results_4.back(), labels_with_parents));

    auto results_5 = fp_dijkstra(0, 6, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_4{0, 3, 6};
    REQUIRE(reference_path_4 == common::get_path(0, 6, results_5.front(), labels_with_parents));

    auto results_6 = fp_dijkstra(0, 5, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_5{0, 1, 2, 5};
    std::vector<TestGraph::node_id_t> reference_path_6{0, 4, 5};
    REQUIRE(reference_path_5 == common::get_path(0, 5, results_6.front(), labels_with_parents));
    REQUIRE(reference_path_6 == common::get_path(0, 5, results_6.back(), labels_with_parents));
}

TEST_CASE("Reference graph searches with FP", "[mc dijkstra]") {
    //                        8
    //                        |
    //   0-----1-----2----4---7---9
    //    \   /      |    |   |
    //     \ /       |    |   10
    //      3        5----6
    TestGraph graph{11, std::vector<TestGraph::edge_t>{{0, 1,  ev::make_constant(1, 1)},   //
                                                       {0, 3,  ev::make_constant(1, 1)},   //
                                                       {1, 0,  ev::make_constant(2, 1)},   //
                                                       {1, 2,  ev::make_constant(2, 1)},   //
                                                       {1, 3,  ev::make_constant(2, 1)},   //
                                                       {2, 1,  ev::make_constant(3, 1)},   //
                                                       {2, 4,  ev::make_constant(3, 1)},   //
                                                       {2, 5,  ev::make_constant(3, 1)},   //
                                                       {3, 0,  ev::make_constant(4, 1)},   //
                                                       {3, 1,  ev::make_constant(4, 1)},   //
                                                       {4, 2,  ev::make_constant(5, 1)},   //
                                                       {4, 6,  ev::make_constant(5, 1)},   //
                                                       {4, 7,  ev::make_constant(5, 1)},   //
                                                       {5, 2,  ev::make_constant(6, 1)},   //
                                                       {5, 6,  ev::make_constant(20, 1)},  //
                                                       {6, 4,  ev::make_constant(7, 1)},   //
                                                       {6, 5,  ev::make_constant(7, 1)},   //
                                                       {7, 4,  ev::make_constant(8, 1)},   //
                                                       {7, 8,  ev::make_constant(8, 1)},   //
                                                       {7, 9,  ev::make_constant(8, 1)},   //
                                                       {7, 10, ev::make_constant(8, 1)},  //
                                                       {8, 7,  ev::make_constant(9, 1)},   //
                                                       {9, 7,  ev::make_constant(10, 1)},  //
                                                       {10, 7, ev::make_constant(11, 1)}}};

    MinIDQueue queue(graph.num_nodes());
    TestLabels labels(graph.num_nodes());

    // 0 -> 9:  0-1-2-4-7-9 (1,2,3,5,8)
    // 5 -> 6:  5-2-4-6     (6,3,5)
    // 3 -> 8:  3-1-2-4-7-8 (4,2,3,5,8)
    // 3 -> 6:  3-1-2-4-6   (4,2,3,5)
    // 9 -> 5:  9-7-4-2-5   (10,8,5,3)
    // 10 -> 9: 10-7-9      (11,8)
    // 2 -> 4:  2-4         (3)
    // 2 -> 2:  2           ()

    auto results_1 = fp_dijkstra(0, 9, graph, queue, labels);
    std::vector<TestLabelEntry> reference_1{{19, ev::make_constant(19, 5)}};
    REQUIRE(results_1 == reference_1);

    auto results_2 = fp_dijkstra(5, 6, graph, queue, labels);
    std::vector<TestLabelEntry> reference_2{{14, ev::make_constant(14, 3)}, {20, ev::make_constant(20, 1)}};
    REQUIRE(results_2 == reference_2);

    auto results_3 = fp_dijkstra(3, 8, graph, queue, labels);
    std::vector<TestLabelEntry> reference_3{{22, ev::make_constant(22, 5)}};
    REQUIRE(results_3 == reference_3);

    auto results_4 = fp_dijkstra(3, 6, graph, queue, labels);
    std::vector<TestLabelEntry> reference_4{{14, ev::make_constant(14, 4)}};
    REQUIRE(results_4 == reference_4);

    auto results_5 = fp_dijkstra(9, 5, graph, queue, labels);
    std::vector<TestLabelEntry> reference_5{{26, ev::make_constant(26, 4)}};
    REQUIRE(results_5 == reference_5);

    auto results_6 = fp_dijkstra(10, 9, graph, queue, labels);
    std::vector<TestLabelEntry> reference_6{{19, ev::make_constant(19, 2)}};
    REQUIRE(results_6 == reference_6);

    auto results_7 = fp_dijkstra(2, 4, graph, queue, labels);
    std::vector<TestLabelEntry> reference_7{{3, ev::make_constant(3, 1)}};
    REQUIRE(results_7 == reference_7);

    auto results_8 = fp_dijkstra(2, 2, graph, queue, labels);
    std::vector<TestLabelEntry> reference_8{{0, ev::make_constant(0, 0)}};
    REQUIRE(results_8 == reference_8);

    TestLabelsWithParents labels_with_parents(graph.num_nodes());

    auto results_9 = fp_dijkstra(0, 9, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_1{0, 1, 2, 4, 7, 9};
    REQUIRE(reference_path_1 == common::get_path(0, 9, results_9.front(), labels_with_parents));

    auto results_10 = fp_dijkstra(5, 6, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_2{5, 2, 4, 6};
    std::vector<TestGraph::node_id_t> reference_path_8{5, 6};
    REQUIRE(reference_path_2 == common::get_path(5, 6, results_10.front(), labels_with_parents));
    REQUIRE(reference_path_8 == common::get_path(5, 6, results_10.back(), labels_with_parents));

    auto results_11 = fp_dijkstra(3, 8, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_3{3, 1, 2, 4, 7, 8};
    REQUIRE(reference_path_3 == common::get_path(3, 8, results_11.front(), labels_with_parents));

    auto results_12 = fp_dijkstra(3, 6, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_4{3, 1, 2, 4, 6};
    REQUIRE(reference_path_4 == common::get_path(3, 6, results_12.front(), labels_with_parents));

    auto results_13 = fp_dijkstra(9, 5, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_5{9, 7, 4, 2, 5};
    REQUIRE(reference_path_5 == common::get_path(9, 5, results_13.front(), labels_with_parents));

    auto results_14 = fp_dijkstra(10, 9, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_6{10, 7, 9};
    REQUIRE(reference_path_6 == common::get_path(10, 9, results_14.front(), labels_with_parents));

    auto results_15 = fp_dijkstra(2, 4, graph, queue, labels_with_parents);
    std::vector<TestGraph::node_id_t> reference_path_7{2, 4};
    REQUIRE(reference_path_7 == common::get_path(2, 4, results_15.front(), labels_with_parents));

    std::vector<TestLabelEntryWithParent> reference_labels_1{
        {0, {ev::make_constant(0, 0)}, InterpolatingIncFunction{{LimitedLinearFunction{0, 0, ConstantFunction{0}}}}, INVALID_ID, INVALID_ID}, 
        {3, {ev::make_constant(3, 1)}, InterpolatingIncFunction{{LimitedLinearFunction{3, 3, ConstantFunction{0}}}}, 2, 0}};
    std::vector<TestLabelEntryWithParent> result_labels_1;
    std::tie(std::ignore, result_labels_1) = common::get_path_with_labels(2, 4, results_15.front(), labels_with_parents);
    REQUIRE(reference_labels_1 == result_labels_1);
}
