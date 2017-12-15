#ifndef CHARGE_SERVER_HTTP_HPP
#define CHARGE_SERVER_HTTP_HPP

#include "server/charge.hpp"
#include "server/to_json.hpp"

#include <web++.hpp>

#include <optional>
#include <thread>

namespace charge::server {
namespace detail {

inline void error(WPP::Response *res, const std::string &msg) {
    res->body << "{\"error\":\"" << msg << "\"}";
    res->code = 400;
}

template <typename T>
std::optional<T> param(WPP::Request *req, WPP::Response *res, const std::string &name,
                       bool optional = false);

template <>
inline std::optional<std::string> param<std::string>(WPP::Request *req, WPP::Response *res,
                                                     const std::string &name, bool optional) {
    auto iter = req->query.find(name);
    if (iter == req->query.end()) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return iter->second;
}

template <>
inline std::optional<bool> param<bool>(WPP::Request *req, WPP::Response *res,
                                                     const std::string &name, bool optional) {
    auto iter = req->query.find(name);
    if (iter == req->query.end()) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    if (iter->second == "true")
        return true;
    else if (iter->second == "false")
        return false;
    else
    {
        error(res, "Parameter has invalid value: " + name + " = " + iter->second);
        return {};
    }
}

template <>
inline std::optional<int> param<int>(WPP::Request *req, WPP::Response *res, const std::string &name,
                                     bool optional) {
    auto iter = req->query.find(name);
    if (iter == req->query.end()) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return std::stoi(iter->second);
}

template <>
inline std::optional<Charge::Algorithm>
param<Charge::Algorithm>(WPP::Request *req, WPP::Response *res, const std::string &name,
                         bool optional) {
    auto iter = req->query.find(name);
    if (iter == req->query.end()) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    if (iter->second == "fastest_bi_dijkstra") {
        return Charge::Algorithm::FASTEST_BI_DIJKSTRA;
    } else if (iter->second == "mc_dijkstra") {
        return Charge::Algorithm::MC_DIJKSTRA;
    } else if (iter->second == "mcc_dijkstra") {
        return Charge::Algorithm::MCC_DIJKSTRA;
    } else if (iter->second == "fp_dijkstra") {
        return Charge::Algorithm::FP_DIJKSTRA;
    } else if (iter->second == "fpc_dijkstra") {
        return Charge::Algorithm::FPC_DIJKSTRA;
    } else if (iter->second == "fpc_profile_dijkstra") {
        return Charge::Algorithm::FPC_PROFILE_DIJKSTRA;
    }

    error(res, "Unknown algorithm: " + iter->second);
    return {};
}

template <>
inline std::optional<double> param<double>(WPP::Request *req, WPP::Response *res,
                                           const std::string &name, bool optional) {
    auto iter = req->query.find(name);
    if (iter == req->query.end()) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return std::stof(iter->second);
}

inline void handle_route(const Charge &charge, WPP::Request *req, WPP::Response *res) {
    auto algorithm = param<Charge::Algorithm>(req, res, "algorithm");
    auto start = param<int>(req, res, "start");
    auto target = param<int>(req, res, "target");
    if (!algorithm || !start || !target)
        return;

    auto search_space = param<bool>(req, res, "search_space", true);
    if (!search_space)
        search_space = false;

    auto routes = charge.route(*algorithm, *start, *target, *search_space);
    if (routes.empty()) {
        error(res, "No route found.");
    } else {
        res->body << to_json(*start, *target, routes);
    }
}

inline void handle_nearest(const Charge &charge, WPP::Request *req, WPP::Response *res) {
    auto lat = param<double>(req, res, "lat");
    auto lon = param<double>(req, res, "lon");
    if (!lat || !lon)
        return;

    auto nearest = charge.nearest(common::Coordinate::from_floating(*lon, *lat));
    res->body << to_json(nearest);
}
} // namespace detail

class HTTPServer {
  public:
    HTTPServer(const Charge &charge, unsigned port) {
        server.get("/route", [&](WPP::Request *req, WPP::Response *res) {
            detail::handle_route(charge, req, res);
        });
        server.get("/nearest", [&](WPP::Request *req, WPP::Response *res) {
            detail::handle_nearest(charge, req, res);
        });

        worker_thread = std::thread([this, port]() {
            try {
                server.start(port);
            } catch (WPP::Exception e) {
                std::cerr << "WebServer: " << e.what() << std::endl;
            }
        });
    }

    void stop() {
        server.stop();
        worker_thread.join();
    }

    void wait() { worker_thread.join(); }

  private:
    WPP::Server server;
    std::thread worker_thread;
};
} // namespace charge::server

#endif
