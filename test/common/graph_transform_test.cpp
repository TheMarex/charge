#include "common/adj_graph.hpp"
#include "common/constant_function.hpp"
#include "common/function_graph.hpp"
#include "common/graph_transform.hpp"
#include "common/linear_function.hpp"
#include "common/turn_cost_model.hpp"

#include "../helper/function_printer.hpp"

#include <catch.hpp>

namespace Catch {
template <>
inline std::string toString<charge::common::FunctionGraph<charge::common::LinearFunction>::edge_t>(
    const charge::common::FunctionGraph<charge::common::LinearFunction>::edge_t &e) {
    std::stringstream ss;
    ss << "{ " << e.start << ", " << e.target << ", " << toString(e.weight) << "}";
    return ss.str();
}
} // namespace Catch

using namespace charge;
using namespace charge::common;

TEST_CASE("Computing undirected degree test", "[graph transform]") {
    std::vector<AdjGraph::edge_t> input_edges{{0, 1}, {1, 0}, {2, 0}, {2, 1}, {2, 3}, {3, 2}};
    auto graph = AdjGraph{4, input_edges};

    auto degree = computeDegree(common::toUndirected(graph));

    CHECK(degree[0] == 2);
    CHECK(degree[1] == 2);
    CHECK(degree[2] == 3);
    CHECK(degree[3] == 1);
}

TEST_CASE("Computing turn graph", "[graph transform]") {
    using Graph = FunctionGraph<LinearFunction>;
    std::vector<Graph::edge_t> input_edges{{0, 1, {0, 0, ConstantFunction{0}}}, //
                                           {1, 0, {1, 1, ConstantFunction{0}}}, //
                                           {1, 2, {2, 2, ConstantFunction{0}}}, //
                                           {2, 4, {3, 3, ConstantFunction{0}}}, //
                                           {2, 5, {4, 4, ConstantFunction{0}}}, //
                                           {3, 2, {5, 5, ConstantFunction{0}}}, //
                                           {4, 2, {6, 6, ConstantFunction{0}}}, //
                                           {5, 2, {7, 7, ConstantFunction{0}}}};
    auto graph = Graph{6, input_edges};

    ZeroTurnCostModel<Graph> model;

    auto turn_graph = toTurnGraph(graph, model);
    auto turns = turn_graph.edges();

    std::vector<Graph::edge_t> reference_turns{
        {0, 2, {0, 0,   ConstantFunction{0}}},   //
        {1, 0, {21, 21, LinearFunction{0, 20, 0}}}, //
        {2, 3, {2, 2,   ConstantFunction{0}}},   //
        {2, 4, {2, 2,   ConstantFunction{0}}},   //
        {3, 6, {23, 23, LinearFunction{0, 20, 0}}}, //
        {4, 7, {24, 24, LinearFunction{0, 20, 0}}},   //
        {5, 3, {5, 5,   ConstantFunction{0}}},   //
        {5, 4, {5, 5,   ConstantFunction{0}}},   //
        {6, 3, {26, 26, LinearFunction{0, 20, 0}}}, //
        {6, 4, {6, 6,   ConstantFunction{0}}},   //
        {7, 3, {7, 7,   ConstantFunction{0}}}, //
        {7, 4, {27, 27, LinearFunction{0, 20, 0}}},
    };

    CHECK(turns == reference_turns);
}
