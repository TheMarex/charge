#include "ev/graph_transform.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::ev;

TEST_CASE("Compute omega graph", "[tradeoff_to_omega_graph]") {
    // Path from 0 to 3 is the fastest but not
    // if you include the charging station at (5)
    // and battery constrains.
    //
    // 0->3 : duration >= 8, consumption >= (300 + 200 - 350)
    //
    // 0 -> 1 -> 2 -> 3
    // |              |
    // 4------(5)-----6
    //
    ev::TradeoffGraph graph(7, std::vector<ev::TradeoffGraph::edge_t>{
                                   {0, 1, ev::make_constant(3, 300)},
                                   {0, 4, ev::make_constant(6, 100)},
                                   {1, 2, {3, 7, common::HyperbolicFunction{2500, 2, 100}}},
                                   {2, 3, {2, 3, common::HyperbolicFunction{1000, 1, -600}}},
                                   {4, 5, ev::make_constant(6, 100)},
                                   {5, 6, ev::make_constant(6, 100)},
                                   {6, 3, ev::make_constant(6, 100)}});

    // the weight of every edge should be a lower bound on the time
    // it takes to travel plus charging
    auto omega_graph = ev::tradeoff_to_omega_graph(graph, -100);

    auto weight_01 = omega_graph.weight(omega_graph.edge(0, 1));
    auto weight_12 = omega_graph.weight(omega_graph.edge(1, 2));
    auto weight_23 = omega_graph.weight(omega_graph.edge(2, 3));
    auto weight_04 = omega_graph.weight(omega_graph.edge(0, 4));
    auto weight_45 = omega_graph.weight(omega_graph.edge(4, 5));
    auto weight_56 = omega_graph.weight(omega_graph.edge(5, 6));
    auto weight_63 = omega_graph.weight(omega_graph.edge(6, 3));
    CHECK(weight_01 == common::to_fixed(6));
    // has slope of -100 at about x=5.68
    CHECK(weight_12 == Approx(common::to_fixed(5.684 + 284.2 / 100.0)));
    CHECK(weight_23 == Approx(common::to_fixed(3 - 350 / 100.0)));
    CHECK(weight_04 == Approx(common::to_fixed(6 + 1)));
    CHECK(weight_45 == Approx(common::to_fixed(6 + 1)));
    CHECK(weight_56 == Approx(common::to_fixed(6 + 1)));
    CHECK(weight_63 == Approx(common::to_fixed(6 + 1)));
}

TEST_CASE("Potential shifting") {
    //     2   4
    //    / \ / \
    //   1   3   \
    //  /         \
    // 0-----------5
    //
    ev::TradeoffGraph graph(6, std::vector<ev::TradeoffGraph::edge_t>{
                                   {0, 1, ev::make_constant(3, 300)},
                                   {1, 2, ev::make_constant(6, 300)},
                                   {2, 3, {2, 3, common::HyperbolicFunction{1000, 1, -300}}},
                                   {3, 4, {2, 3, common::HyperbolicFunction{1000, 1, 400}}},
                                   {4, 5, ev::make_constant(6, -500)},
                                   {5, 0, ev::make_constant(6, 100)},
                               });

    std::vector<std::int32_t> heights = {0, 1, 2, 1, 2, 0};

    auto shifted = ev::tradeoff_to_min_consumption(graph);
    auto potentials = ev::shift_negative_weights(shifted, heights);

    auto weight_01 = shifted.weight(shifted.edge(0, 1));
    auto weight_12 = shifted.weight(shifted.edge(1, 2));
    auto weight_23 = shifted.weight(shifted.edge(2, 3));
    auto weight_34 = shifted.weight(shifted.edge(3, 4));
    auto weight_45 = shifted.weight(shifted.edge(4, 5));
    auto weight_50 = shifted.weight(shifted.edge(5, 0));
    CHECK(Approx(weight_01 - potentials[0] + potentials[1]) == common::to_fixed(300));
    CHECK(Approx(weight_12 - potentials[1] + potentials[2]) == common::to_fixed(300));
    CHECK(Approx(weight_23 - potentials[2] + potentials[3]) == common::to_fixed(-50));
    CHECK(Approx(weight_34 - potentials[3] + potentials[4]) == common::to_fixed(650));
    CHECK(Approx(weight_45 - potentials[4] + potentials[5]) == common::to_fixed(-500));
    CHECK(Approx(weight_50 - potentials[5] + potentials[0]) == common::to_fixed(100));

    auto path_weight = weight_01 + weight_12 + weight_23 + weight_34 + weight_45;
    CHECK(Approx(path_weight - potentials[0] + potentials[5]) == common::to_fixed(700));
}
