#include "preprocessing/import_charger_geojson.hpp"

#include "ev/files.hpp"

#include "common/files.hpp"
#include "common/timed_logger.hpp"
#include "common/to_geojson.hpp"

#include <json.hpp>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << argv[0] << " PATH_TO_GEOJSON BASE_GRAPH_PATH BASE_OUTPUT_PATH" << std::endl;
        std::cout << "Example: " << argv[0] << " data/chargers.geojson data/luxev cache/luxev"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string geojson_path = argv[1];
    const std::string network_base_path = argv[2];
    const std::string output_base_path = argv[3];

    using namespace charge;

    common::TimedLogger time_read_chargemap("Reading charging stations");
    nlohmann::json geojson;
    {
        std::ifstream stream(geojson_path);
        stream >> geojson;
    }
    time_read_chargemap.finished();

    common::TimedLogger time_read_coordinates("Reading coordinates");
    auto coordinates = common::files::read_coordinates(network_base_path);
    time_read_coordinates.finished();

    common::TimedLogger time_convert("Converting to chargers");
    auto chargers = preprocessing::charger_from_geojson(geojson, coordinates);
    time_convert.finished();

    std::unordered_map<double, std::size_t> rate_counts;
    for (auto rate : chargers)
        rate_counts[rate]++;

    {
        auto fc = common::chargers_to_geojson(chargers, coordinates);
        std::ofstream writer(output_base_path + "/charger.geojson");
        writer << fc;
    }

    for (const auto& pair : rate_counts)
    {
        auto [rate, count] = pair;
        if (rate > 0)
        {
            std::cerr << rate/1000. << " kW: " << count << std::endl;
        }
    }

    ev::files::write_charger(output_base_path, chargers);

    return EXIT_SUCCESS;
}
