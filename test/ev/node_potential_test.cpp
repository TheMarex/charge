#include "ev/node_potentials.hpp"
#include "ev/graph_transform.hpp"
#include "common/graph_transform.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::ev;

TEST_CASE("Check omega potential", "[omega potential]")
{
    // Path from 0 to 3 is the fastest but not
    // if you include the charging station at (5)
    // and battery constrains.
    //
    // 0->3 : duration >= 8, consumption >= (200 + 200 - 350)
    //
    // 0 -> 1 -> 2 -> 3
    // |              |
    // 4------(5)-----6
    //
    ev::TradeoffGraph graph(7, std::vector<ev::TradeoffGraph::edge_t> {
            {0, 1, ev::make_constant(3, 200)},
            {0, 4, ev::make_constant(6, 100)},
            {1, 2, {3, 7, common::HyperbolicFunction {2500, 2, 100}}},
            {2, 3, {2, 3, common::HyperbolicFunction {1000, 1, -600}}},
            {4, 5, ev::make_constant(6, 100)},
            {5, 6, ev::make_constant(6, 100)},
            {6, 3, ev::make_constant(6, 100)}
    });

    const auto capacity = 300;
    const auto min_charging_rate = -100;

    auto reverse_duration_graph = common::invert(ev::tradeoff_to_min_duration(graph));
    auto reverse_consumption_graph = common::invert(ev::tradeoff_to_min_consumption(graph));
    auto reverse_omega_graph = common::invert(ev::tradeoff_to_omega_graph(graph, min_charging_rate));

    OmegaNodePotentials potential {capacity, min_charging_rate, reverse_duration_graph, reverse_consumption_graph, reverse_omega_graph};

    common::MinIDQueue queue(graph.num_nodes());
    potential.recompute(queue, 3);

    ev::PiecewieseTradeoffFunction path_01 {{
        graph.weight(graph.edge(0, 1))
    }};

    ev::PiecewieseTradeoffFunction path_12 {{
        {10, 10, common::HyperbolicFunction {2500, 5, 300}}
    }};

    ev::PiecewieseTradeoffFunction path_04 {{
        graph.weight(graph.edge(0, 4))
    }};

    ev::PiecewieseTradeoffFunction path_45 {{
        ev::make_constant(12, 200)
    }};

    ev::PiecewieseTradeoffFunction path_56 {{
        ev::make_constant(18, 300)
    }};

    ev::PiecewieseTradeoffFunction path_63 {{
        ev::make_constant(24, 400)
    }};

    // Simulate dijkstra query from 0 to 3
    CHECK(potential.key(0, 0, ev::make_constant(0, 0)) == 8000);
    CHECK(potential.key(1, 3000, path_01) == 8000);
    CHECK(potential.key(4, 6000, path_04) == 25000);
    CHECK(potential.key(5, 12000, path_45) == 25000);
    CHECK(potential.key(6, 18000, path_56) == 25000);
    CHECK(potential.key(3, 24000, path_63) == 25000);
}
