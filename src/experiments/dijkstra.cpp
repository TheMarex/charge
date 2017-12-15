#include "experiments/experiment_runner.hpp"
#include "experiments/files.hpp"

#include "common/files.hpp"
#include "common/graph_transform.hpp"
#include "common/timed_logger.hpp"
#include "ev/dijkstra.hpp"

#include "ev/graph_transform.hpp"

#include <string>

int main(int argc, char **argv) {
    if (argc < 6) {
        std::cerr << argv[0] << " EXPERIMENT_PATH NUM_RUNS THREADS LOG_PATH GRAPH_BASE_PATH"
                  << std::endl;
        std::cerr << "Example:" << argv[0] << " 10 10000 2 results/random_dijkstra data/luxev"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string experiment_path = argv[1];
    const std::size_t num_runs = std::stoi(argv[2]);
    const std::size_t threads = std::stoi(argv[3]);
    const std::string experiment_log = argv[4];
    const std::string graph_base = argv[5];

    using namespace charge;
    common::TimedLogger load_timer("Loading graph");
    const auto tradeoff_graph = ev::TradeoffGraph{
        common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(graph_base)};
    const auto min_consumption_graph = ev::tradeoff_to_min_consumption(tradeoff_graph);
    const auto graph = ev::tradeoff_to_min_duration(tradeoff_graph);
    const auto heights = common::files::read_heights(graph_base);
    const auto coordinates = common::files::read_coordinates(graph_base);
    const auto queries = experiments::files::read_queries(experiment_path);
    load_timer.finished();

    std::cerr << "Graph has " << graph.num_nodes() << " nodes and " << graph.num_edges()
              << " edges." << std::endl;

    common::TimedLogger setup_timer("Setting up experiment");

    experiments::ResultLogger result_logger{experiment_log + ".json", coordinates, heights};
    auto runner = experiments::make_experiment_runner(
        ev::MinDurationDijkstraContetx{tradeoff_graph, graph}, std::move(queries), experiment_log,
        result_logger, num_runs);

    setup_timer.finished();

    runner.run(threads);
    runner.summary();

    return EXIT_SUCCESS;
}
