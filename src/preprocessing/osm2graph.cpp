#include "preprocessing/import_osm.hpp"

#include "common/files.hpp"
#include "common/graph_statistics.hpp"
#include "common/timed_logger.hpp"
#include "common/weighted_graph.hpp"

#include "ev/graph_transform.hpp"
#include "ev/phem.hpp"

#include <iostream>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << argv[0] << " PATH_TO_NETWORK PATH_TO_PHEM_MODEL PATH_TO_SRTM OUTPUT_PATH"
                  << std::endl;
        std::cout << "Example: " << argv[0]
                  << " data/berlin.osm.pbf data/phem data/srtm cache/luxev" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string network_base_path = argv[1];
    const std::string phem_base_path = argv[2];
    const std::string srtm_base_path = argv[3];
    const std::string output_base_path = argv[4];

    charge::common::TimedLogger time_read("Reading network from");
    auto network = charge::preprocessing::read_network(network_base_path);
    time_read.finished();

    charge::common::TimedLogger time_srtm("Annotating SRTM elevation");
    charge::preprocessing::annotate_elevation(srtm_base_path, network);
    time_srtm.finished();

    charge::common::TimedLogger time_simplify("Simplify OSM data");
    auto nodes_before = network.nodes.size();
    network = charge::preprocessing::simplify_network(std::move(network));
    auto nodes_after = network.nodes.size();
    time_simplify.finished();
    std::cerr << "Removed " << (nodes_before - nodes_after) << "(" << ((nodes_before - nodes_after) / (double)nodes_before * 100) << "%) nodes." << std::endl;

    charge::common::TimedLogger time_phem("Importing PHEM model");
    auto models = charge::ev::import_phem_consumption_models(phem_base_path);
    time_phem.finished();

    charge::common::TimedLogger time_convert("Converting to graph");
    auto graph =
        charge::preprocessing::tradeoff_graph_from_network(network, *models["Peugeot Ion"]);
    auto coordinates = charge::preprocessing::coordinates_from_network(network);
    auto heights = charge::preprocessing::heights_from_network(network);
    time_convert.finished();

    std::cerr << charge::common::get_statistics(graph) << std::endl;

    charge::common::TimedLogger time_write("Writing graph");
    charge::common::files::write_weighted_graph(
        output_base_path, static_cast<const charge::ev::TradeoffGraph::Base &>(graph));
    charge::common::files::write_coordinates(output_base_path, coordinates);
    charge::common::files::write_heights(output_base_path, heights);
    time_write.finished();

    return EXIT_SUCCESS;
}
