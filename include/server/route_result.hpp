#ifndef CHARGE_SERVER_ROUTE_RESULT_HPP
#define CHARGE_SERVER_ROUTE_RESULT_HPP

#include "common/coordinate.hpp"
#include "common/search_space.hpp"

#include "ev/limited_tradeoff_function.hpp"

#include <vector>
#include <cstdint>

namespace charge::server
{
struct RouteResult {
    ev::PiecewieseTradeoffFunction tradeoff;   // maps duration in s to consumption in mWh
    std::vector<double> durations;              // durations in s
    std::vector<double> max_speeds;             // maximal speed in km/h
    std::vector<double> consumptions;           // consumption in Wh
    std::vector<double> lengths;                // lengths in meters
    std::vector<std::uint32_t> path;           // node ids of the graph
    std::vector<common::Coordinate> geometry;  // lon,lat coordinates
    std::vector<std::int32_t> heights;         // height in meters
    std::vector<common::SearchSpaceNode> search_space; // nodes in the search space
};
}

#endif
