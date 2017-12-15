#include "experiments/experiment_runner.hpp"
#include "experiments/files.hpp"

#include "common/files.hpp"
#include "common/graph_statistics.hpp"
#include "common/graph_transform.hpp"
#include "common/timed_logger.hpp"

#include "ev/files.hpp"
#include "ev/graph_transform.hpp"
#include "ev/mcc_dijkstra.hpp"

#include <string>

int main(int argc, char **argv) {
    if (argc < 12) {
        std::cerr << argv[0]
                  << " EXPERIMENT_PATH NUM_RUNS THREADS LOG_PATH "
                     "GRAPH_BASE_PATH CAPACITY_WH POTENTIAL CONSUMPTION_SAMPLE_RESOLUTION X_EPS Y_EPS "
                     "CHARGING_PENALTY [MAX_TIME]"
                  << std::endl;
        std::cerr << "Example:" << argv[0]
                  << "cache/luxev/random 10 10000 2 results/random_dijkstra data/luxev 16000.0 fastest 6000"
                     "10.0 0.1 1.0 60"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string experiment_path = argv[1];
    const std::size_t num_runs = std::stoi(argv[2]);
    const std::size_t threads = std::stoi(argv[3]);
    const std::string experiment_log = argv[4];
    const std::string graph_base = argv[5];
    const double capacity = std::stof(argv[6]);
    const std::string potential = argv[7];
    const double sample_resolution = std::stof(argv[8]);
    const double x_eps = std::stof(argv[9]);
    const double y_eps = std::stof(argv[10]);
    const double charging_penalty = std::stof(argv[11]);
    double max_time = std::numeric_limits<double>::infinity();
    if (argc > 12 && argv[12] != std::string("infinity"))
        max_time = std::stof(argv[12]);

    using namespace charge;
    common::TimedLogger load_timer("Loading graph");

    const auto tradeoff_graph = ev::TradeoffGraph{
        common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(graph_base)};
    const auto min_consumption_graph = ev::tradeoff_to_min_consumption(tradeoff_graph);
    const auto reverse_min_duration_graph =
        common::invert(ev::tradeoff_to_min_duration(tradeoff_graph));
    const auto graph = ev::tradeoff_to_sampled_consumption(tradeoff_graph, sample_resolution);
    auto chargers = ev::files::read_charger(graph_base);
    ev::ChargingFunctionContainer charging_functions{chargers, ev::ChargingModel{capacity}};
    const auto heights = common::files::read_heights(graph_base);
    const auto coordinates = common::files::read_coordinates(graph_base);
    const auto queries = experiments::files::read_queries(experiment_path);
    load_timer.finished();

    std::cerr << common::get_statistics(graph) << std::endl;

    experiments::ResultLogger result_logger{experiment_log + ".json", coordinates, heights};

    if (potential == "fastest") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::MCCAStarFastestContext{x_eps, y_eps, sample_resolution, capacity, charging_penalty,
                                       graph, charging_functions, reverse_min_duration_graph},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();

        runner.run(threads, max_time);
        runner.summary();
    } else if (potential == "lazy_fastest") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::MCCAStarLazyFastestContext{x_eps, y_eps, sample_resolution, capacity, charging_penalty,
                                       graph, charging_functions, reverse_min_duration_graph},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();

        runner.run(threads, max_time);
        //runner.summary();
    } else if (potential == "none") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::MCCDijkstraContext{x_eps, y_eps, sample_resolution, capacity, charging_penalty,
                                       graph, charging_functions},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();

        runner.run(threads, max_time);
        runner.summary();
    } else {
        throw std::runtime_error("Unknown potential: " + potential);
    }

    return EXIT_SUCCESS;
}
