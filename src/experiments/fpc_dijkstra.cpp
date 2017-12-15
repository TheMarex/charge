#include "experiments/experiment_runner.hpp"
#include "experiments/files.hpp"

#include "common/files.hpp"
#include "common/graph_statistics.hpp"
#include "common/graph_transform.hpp"
#include "common/timed_logger.hpp"

#include "ev/files.hpp"
#include "ev/fpc_dijkstra.hpp"
#include "ev/graph_transform.hpp"

#include <string>

int main(int argc, char **argv) {
    if (argc < 11) {
        std::cerr << argv[0]
                  << " EXPERIMENT_PATH NUM_RUNS THREADS LOG_PATH GRAPH_BASE_PATH CAPACITY "
                     "POTENTIAL X_EPS Y_EPS CHARGING_PENALTY [HEURISTIC] [MAX_TIME]"
                  << std::endl;
        std::cerr << "Example:" << argv[0]
                  << " cache/luxev/random 10 10000 2 results/random_dijkstra data/luxev 16000 "
                     "omega 0.1 1 linear 6000"
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
    const double x_eps = std::stof(argv[8]);
    const double y_eps = std::stof(argv[9]);
    const double charging_penalty = std::stof(argv[10]);
    std::string heuristic = "none";
    if (argc > 11)
        heuristic = argv[11];
    double max_time = std::numeric_limits<double>::infinity();
    if (argc > 12 && argv[12] != std::string("infinity"))
        max_time = std::stof(argv[12]);

    using namespace charge;
    common::TimedLogger load_timer("Loading graph");
    auto graph = ev::TradeoffGraph{
        common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(graph_base)};
    auto chargers = ev::files::read_charger(graph_base);
    ev::ChargingFunctionContainer charging_functions{chargers, ev::ChargingModel{capacity}};
    auto min_charging_rate = charging_functions.get_min_chargin_rate(charging_penalty);
    auto max_charging_rate = charging_functions.get_max_chargin_rate(charging_penalty);
    const auto coordinates = common::files::read_coordinates(graph_base);
    load_timer.finished();

    if (heuristic == "min_rate") {
        std::cerr << "Input " << common::get_statistics(graph);
        std::cerr << "Using rate of " << max_charging_rate << " to clip hyperbolic functions."
                  << std::endl;
        graph = tradeoff_to_limited_rate(std::move(graph), max_charging_rate);
    } else if (heuristic == "linear") {
        graph = tradeoff_to_linear(std::move(graph));
    } else if (heuristic == "only_fast") {
        graph = tradeoff_to_only_fast(std::move(graph), coordinates);
    } else if (heuristic == "no_super_charger") {
        for (auto &rate : chargers) {
            rate = std::min<double>(40000.0, rate);
        }
        charging_functions = ev::ChargingFunctionContainer{chargers, ev::ChargingModel{capacity}};
        min_charging_rate = charging_functions.get_min_chargin_rate(charging_penalty);
        max_charging_rate = charging_functions.get_max_chargin_rate(charging_penalty);
    } else if (heuristic == "no_slow_charger") {
        for (auto &rate : chargers) {
            if (rate < 20000)
                rate = 0;
        }
        charging_functions = ev::ChargingFunctionContainer{chargers, ev::ChargingModel{capacity}};
        min_charging_rate = charging_functions.get_min_chargin_rate(charging_penalty);
        max_charging_rate = charging_functions.get_max_chargin_rate(charging_penalty);
    } else if (heuristic == "no_slow_charger_min_rate") {
        for (auto &rate : chargers) {
            if (rate < 20000)
                rate = 0;
        }
        charging_functions = ev::ChargingFunctionContainer{chargers, ev::ChargingModel{capacity}};
        min_charging_rate = charging_functions.get_min_chargin_rate(charging_penalty);
        max_charging_rate = charging_functions.get_max_chargin_rate(charging_penalty);

        std::cerr << "Input " << common::get_statistics(graph);
        std::cerr << "Using rate of " << max_charging_rate << " to clip hyperbolic functions."
                  << std::endl;
        graph = tradeoff_to_limited_rate(std::move(graph), max_charging_rate);
    } else if (heuristic != "none") {
        throw std::runtime_error("Invalid heuristic: " + heuristic);
    }

    std::cout << "Using a charging rates between " << min_charging_rate << "," << max_charging_rate
              << std::endl;

    common::TimedLogger setup_timer("Loading data");
    const auto heights = common::files::read_heights(graph_base);
    /*const*/ auto min_consumption_graph = ev::tradeoff_to_min_consumption(graph);
    const auto reverse_min_duration_graph = common::invert(ev::tradeoff_to_min_duration(graph));
    /*const*/ auto reverse_consumption_graph = common::invert(min_consumption_graph);
    /*const*/ auto omega_graph = ev::tradeoff_to_omega_graph(graph, min_charging_rate);
    /*const*/ auto reverse_omega_graph = common::invert(omega_graph);
    const auto queries = experiments::files::read_queries(experiment_path);
    setup_timer.finished();

    std::cerr << common::get_statistics(graph);

    experiments::ResultLogger result_logger{experiment_log + ".json", coordinates, heights};

    if (potential == "omega") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::FPCAStarOmegaContext{x_eps, y_eps, capacity, charging_penalty, min_charging_rate,
                                     graph, charging_functions, reverse_min_duration_graph,
                                     reverse_consumption_graph, reverse_omega_graph},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();
        runner.run(threads, max_time);
        runner.summary();
    } else if (potential == "fastest") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::FPCAStarFastestContext{x_eps, y_eps, capacity, charging_penalty, graph,
                                       charging_functions, reverse_min_duration_graph},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();
        runner.run(threads, max_time);
        runner.summary();
    } else if (potential == "lazy_omega") {
        auto shifted_omega_potentials = ev::shift_negative_weights(omega_graph, heights);
        auto shifted_consumption_potentials = ev::shift_negative_weights(min_consumption_graph, heights);
        reverse_omega_graph = common::invert(omega_graph);
        reverse_consumption_graph = common::invert(min_consumption_graph);
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::FPCAStarLazyOmegaContext{
                x_eps, y_eps, capacity, charging_penalty, min_charging_rate, graph,
                charging_functions, reverse_min_duration_graph, reverse_consumption_graph,
                shifted_consumption_potentials, reverse_omega_graph, shifted_omega_potentials},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();
        runner.run(threads, max_time);
        runner.summary();
    } else if (potential == "lazy_fastest") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::FPCAStarLazyFastestContext{x_eps, y_eps, capacity, charging_penalty, graph,
                                           charging_functions, reverse_min_duration_graph},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();
        runner.run(threads, max_time);
        runner.summary();
    } else if (potential == "none") {
        common::TimedLogger setup_timer("Setting up experiment");
        auto runner = experiments::make_experiment_runner(
            ev::FPCDijkstraContext{x_eps, y_eps, capacity, charging_penalty, graph,
                                   charging_functions},
            std::move(queries), experiment_log, result_logger, num_runs);
        setup_timer.finished();
        runner.run(threads, max_time);
        runner.summary();
    } else {
        throw std::runtime_error("Unknown potential name: " + potential);
    }

    return EXIT_SUCCESS;
}
