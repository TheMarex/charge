#ifndef CHARGE_SERVER_TO_JSON_HPP
#define CHARGE_SERVER_TO_JSON_HPP

#include "common/to_geojson.hpp"

#include "ev/limited_tradeoff_function.hpp"

#include "server/nearest_result.hpp"
#include "server/route_result.hpp"

#include <json.hpp>
using json = nlohmann::json;

namespace charge::server {

auto to_json(const ev::LimitedTradeoffFunction &tradeoff) {
    json j;
    j["min_duration"] = tradeoff.min_x;
    j["max_duration"] = tradeoff.max_x;
    if (tradeoff->is_linear()) {
        const auto &lin = static_cast<const common::LinearFunction &>(*tradeoff);
        j["a"] = 0;
        j["b"] = lin.b;
        j["c"] = lin.c;
        j["d"] = lin.d;
    } else {
        const auto &hyp = static_cast<const common::HyperbolicFunction &>(*tradeoff);
        assert(tradeoff->is_hyperbolic());
        j["a"] = hyp.a;
        j["b"] = hyp.b;
        j["c"] = hyp.c;
        j["d"] = 0;
    }
    return j;
}

auto to_json(const std::int32_t start, const std::int32_t target, const std::vector<RouteResult> &routes) {
    auto results = json::array();
    for (const auto &route : routes) {
        json j;
        j["path"] = route.path;
        j["durations"] = route.durations;
        auto tradeoff = json::array();
        for (const auto &t : route.tradeoff.functions)
            tradeoff.push_back(to_json(t));
        j["tradeoff"] = tradeoff;
        j["consumptions"] = route.consumptions;
        j["heights"] = route.heights;
        j["lengths"] = route.lengths;
        j["max_speeds"] = route.max_speeds;
        auto geometry = json::array();
        for (const auto c : route.geometry) {
            auto[lon, lat] = c.to_floating();
            geometry.push_back(json::array({lon, lat}));
        }
        j["geometry"] = geometry;
        j["search_space"] = common::search_space_to_geojson(route.search_space);
        results.push_back(j);
    }

    json response;
    response["routes"] = results;
    response["start"] = start;
    response["target"] = target;
    return response;
}

auto to_json(NearestResult nearest) {
    json j;
    j["id"] = nearest.id;
    auto[lon, lat] = nearest.coordinate.to_floating();
    j["coordinate"] = json::array({lon, lat});
    return j;
}
}

#endif
