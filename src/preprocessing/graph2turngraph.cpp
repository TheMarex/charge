#include "common/graph_transform.hpp"
#include "common/timed_logger.hpp"
#include "common/files.hpp"
#include "common/turn_cost_model.hpp"

#include "ev/graph.hpp"
#include "ev/graph_transform.hpp"

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr
            << argv[0]
            << "TURN_COST_MODEL IN_GRAPH_BASE_PATH OUT_GRAPH_BASE_PATH"
            << std::endl;
        std::cerr << "Example:" << argv[0]
                  << "data/luxev data/turnluxev"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string turn_cost_model_name = argv[1];
    const std::string in_graph_base = argv[2];
    const std::string out_graph_base = argv[3];
    using namespace charge;

    common::TimedLogger load_timer("Loading graph");
    const auto in_graph = ev::TradeoffGraph{common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(in_graph_base)};
    const auto in_heights = common::files::read_heights(in_graph_base);
    const auto in_coordinates = common::files::read_coordinates(in_graph_base);
    const auto turn_cost_model = common::make_turn_cost_model<ev::TradeoffGraph::Base>(turn_cost_model_name, in_coordinates);
    load_timer.finished();

    common::TimedLogger convert_timer("Convert graph");
    auto out_graph = common::toTurnGraph(in_graph, *turn_cost_model);
    auto edge_to_start_node = edgeToStartNode(in_graph);

    std::vector<common::Coordinate> out_coordinates(out_graph.num_nodes());
    for (auto edge_id : common::irange<std::size_t>(0, out_graph.num_nodes()))
    {
        out_coordinates[edge_id] = in_coordinates[edge_to_start_node[edge_id]];
    }

    std::vector<std::int32_t> out_heights(out_graph.num_nodes());
    for (auto edge_id : common::irange<std::size_t>(0, out_graph.num_nodes()))
    {
        out_heights[edge_id] = in_heights[edge_to_start_node[edge_id]];
    }
    convert_timer.finished();

    common::TimedLogger write_timer("Writing graph");
    common::files::write_weighted_graph(out_graph_base, out_graph);
    common::files::write_heights(out_graph_base, out_heights);
    common::files::write_coordinates(out_graph_base, out_coordinates);
    write_timer.finished();

    return EXIT_SUCCESS;
}
