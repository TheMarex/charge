#include "experiments/experiment_factory.hpp"
#include "experiments/files.hpp"
#include "experiments/rank_queries.hpp"

#include "common/files.hpp"
#include "common/graph_statistics.hpp"
#include "common/timed_logger.hpp"

#include "ev/files.hpp"
#include "ev/graph_transform.hpp"

#include <string>

int main(int argc, char **argv) {
    if (argc < 5) {
        std::cerr << argv[0]
                  << " THREADS GRAPH_BASE_PATH QUERIES_PATH RANK_OUT_PATH"
                  << std::endl;
        std::cerr << "Example:" << argv[0]
                  << " 2 data/luxev cache/luxev/random_10000_16kw.csv data/luxev/rank_random_10000_16kw.csv"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::size_t threads = std::stoi(argv[1]);
    const std::string graph_base = argv[2];
    const std::string queries_in_path = argv[3];
    const std::string ranks_out_path = argv[4];

    using namespace charge;
    common::TimedLogger load_timer("Loading graph");
    const auto graph = ev::TradeoffGraph{
        common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(graph_base)};
    const auto min_duration_graph = ev::tradeoff_to_min_duration(graph);
    load_timer.finished();

    std::cerr << common::get_statistics(graph) << std::endl;

    common::TimedLogger setup_timer("Generating queries");

    auto queries = experiments::files::read_queries(queries_in_path);
    auto ranks = experiments::make_rank(min_duration_graph, queries, threads);
    experiments::files::write_queries(ranks_out_path, ranks);

    setup_timer.finished();

    return EXIT_SUCCESS;
}
