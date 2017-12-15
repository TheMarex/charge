#include "server/charge.hpp"

#include "server/handlers/dijkstra.hpp"
#include "server/handlers/mc_dijkstra.hpp"
#include "server/handlers/mcc_dijkstra.hpp"
#include "server/handlers/fp_dijkstra.hpp"
#include "server/handlers/fpc_dijkstra.hpp"

#include "ev/files.hpp"

#include "common/files.hpp"
#include "common/nearest_neighbour.hpp"

#include <cmath>
#include <numeric>

namespace charge::server {

Charge::Charge(const std::string &base_path, const double capacity)
    : Charge(base_path, capacity,
             {Algorithm::FASTEST_BI_DIJKSTRA, Algorithm::MC_DIJKSTRA, Algorithm::MCC_DIJKSTRA, Algorithm::FP_DIJKSTRA, Algorithm::FPC_DIJKSTRA, Algorithm::FPC_PROFILE_DIJKSTRA}) {}

Charge::Charge(const std::string &base_path, const double capacity,
               const std::set<Algorithm> &algorithms)
    : graph(common::files::read_weighted_graph<ev::TradeoffGraph::weight_t>(base_path)),
      coordinates(common::files::read_coordinates(base_path)), nn(coordinates),
      heights(common::files::read_heights(base_path)),
       charging_functions{ev::files::read_charger(base_path), ev::ChargingModel{capacity}} {

    if (algorithms.count(Algorithm::FASTEST_BI_DIJKSTRA) > 0) {
        handlers[Algorithm::FASTEST_BI_DIJKSTRA] = std::make_shared<handlers::Dijkstra>(graph);
    }
    if (algorithms.count(Algorithm::MC_DIJKSTRA) > 0) {
        handlers[Algorithm::MC_DIJKSTRA] = std::make_shared<handlers::MCDijkstra>(graph, capacity, coordinates);
    }
    if (algorithms.count(Algorithm::MCC_DIJKSTRA) > 0) {
        handlers[Algorithm::MCC_DIJKSTRA] = std::make_shared<handlers::MCCDijkstra>(graph, capacity, charging_functions, coordinates);
    }
    if (algorithms.count(Algorithm::FP_DIJKSTRA) > 0) {
        handlers[Algorithm::FP_DIJKSTRA] = std::make_shared<handlers::FPDijkstra>(graph, capacity, coordinates);
    }
    if (algorithms.count(Algorithm::FPC_DIJKSTRA) > 0) {
        handlers[Algorithm::FPC_DIJKSTRA] = std::make_shared<handlers::FPCDijkstra>(graph, capacity, charging_functions, coordinates, heights);
    }
    if (algorithms.count(Algorithm::FPC_PROFILE_DIJKSTRA) > 0) {
        handlers[Algorithm::FPC_PROFILE_DIJKSTRA] = std::make_shared<handlers::FPCProfileDijkstra>(graph, capacity, charging_functions, coordinates, heights);
    }
}

std::vector<RouteResult> Charge::route(Algorithm algo, std::uint32_t start,
                                       std::uint32_t target, bool search_space) const {
    if (auto algo_iter = handlers.find(algo); algo_iter != handlers.end()) {
        auto routes = algo_iter->second->route(start, target, search_space);

        for (auto &route : routes) {
            annotate_heights(route, heights);
            annotate_coordinates(route, coordinates);
            annotate_lengths(route);
            annotate_max_speeds(route, graph);
        }

        return routes;
    } else {
        throw std::runtime_error("Algorithm not found.");
    }

    return {};
}

NearestResult Charge::nearest(common::Coordinate coordinate) const {
    auto id = static_cast<std::uint32_t>(nn.nearest(coordinate));
    return NearestResult{id, coordinates[id]};
}
}
