#ifndef CHARGE_SERVER_SERVER_HPP
#define CHARGE_SERVER_SERVER_HPP

#include "server/handlers/algorithm_handler.hpp"
#include "server/nearest_result.hpp"
#include "server/route_result.hpp"

#include "common/coordinate.hpp"
#include "common/nearest_neighbour.hpp"

#include "ev/graph.hpp"
#include "ev/charging_function_container.hpp"

#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

namespace charge {
namespace server {

// Thread safe wrapper
class Charge {
  public:
    enum Algorithm {
        FASTEST_BI_DIJKSTRA,
        MC_DIJKSTRA,
        MCC_DIJKSTRA,
        FP_DIJKSTRA,
        FPC_DIJKSTRA,
        FPC_PROFILE_DIJKSTRA,
    };

    Charge(const std::string &base_path, const double capacity);
    Charge(const std::string &base_path, const double capacity,
           const std::set<Algorithm> &algorithms);

    std::vector<RouteResult> route(Algorithm algo, std::uint32_t start, std::uint32_t target, bool search_space = false) const;

    NearestResult nearest(common::Coordinate coordinate) const;

  private:
    ev::TradeoffGraph graph;
    std::vector<common::Coordinate> coordinates;
    common::NearestNeighbour<> nn;
    std::vector<std::int32_t> heights;
    ev::ChargingFunctionContainer charging_functions;

    std::unordered_map<Algorithm, std::shared_ptr<handlers::AlgorithmHandler>> handlers;
};
}
}

#endif
