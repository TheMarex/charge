#include "experiments/experiment_factory.hpp"
#include "experiments/files.hpp"

#include "common/files.hpp"
#include "common/graph_statistics.hpp"
#include "common/graph_transform.hpp"
#include "common/timed_logger.hpp"

#include "ev/charging_function_container.hpp"
#include "ev/files.hpp"
#include "ev/graph_transform.hpp"

#include <string>

int main(int argc, char **argv) {
    if (argc < 8) {
        std::cerr << argv[0]
                  << " EXPERIMENT_NAME SEED NUM_QUERIES THREADS GRAPH_BASE_PATH "
                     "CAPACITY QUERIES_OUT_PATH "
                  << std::endl;
        std::cerr << "Example:" << argv[0]
                  << " random 1337 10000 2 data/luxev 16000 data/luxev/random_10000_16kw"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string experiment_name = argv[1];
    const std::size_t seed = std::stoi(argv[2]);
    const std::size_t num_queries = std::stoi(argv[3]);
    const std::size_t threads = std::stoi(argv[4]);
    const std::string graph_base = argv[5];
    const double capacity = std::stof(argv[6]);
    const std::string queries_out_path = argv[7];

    using namespace charge;
    common::TimedLogger load_timer("Loading graph");
    const auto graph = ev::TradeoffGraph{
        common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(graph_base)};
    const auto min_consumption_graph = ev::tradeoff_to_min_consumption(graph);
    const auto max_consumption_graph = ev::tradeoff_to_max_consumption(graph);
    const auto min_duration_graph = ev::tradeoff_to_min_duration(graph);
    auto chargers = ev::files::read_charger(graph_base);
    ev::ChargingFunctionContainer charging_functions{chargers, ev::ChargingModel{capacity}};
    const auto heights = common::files::read_heights(graph_base);
    const auto coordinates = common::files::read_coordinates(graph_base);
    load_timer.finished();

    std::cerr << common::get_statistics(graph) << std::endl;

    common::TimedLogger setup_timer("Generating queries");

    auto queries =
        experiments::make_experiment(experiment_name, seed, num_queries, min_duration_graph, min_consumption_graph,
                                     max_consumption_graph, charging_functions, capacity, threads);
    experiments::files::write_queries(queries_out_path, queries);

    setup_timer.finished();

    return EXIT_SUCCESS;
}
