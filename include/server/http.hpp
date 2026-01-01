#ifndef CHARGE_SERVER_HTTP_HPP
#define CHARGE_SERVER_HTTP_HPP

#include "server/charge.hpp"
#include "server/to_json.hpp"

#include <httplib.hpp>

#include <optional>
#include <sstream>
#include <thread>

namespace charge::server {
namespace detail {

inline void error(httplib::Response &res, const std::string &msg) {
    std::ostringstream ss;
    ss << "{\"error\":\"" << msg << "\"}";
    res.set_content(ss.str(), "application/json");
    res.status = 400;
}

template <typename T>
std::optional<T> param(const httplib::Request &req, httplib::Response &res, const std::string &name,
                       bool optional = false);

template <>
inline std::optional<std::string> param<std::string>(const httplib::Request &req, httplib::Response &res,
                                                     const std::string &name, bool optional) {
    if (!req.has_param(name)) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return req.get_param_value(name);
}

template <>
inline std::optional<bool> param<bool>(const httplib::Request &req, httplib::Response &res,
                                                     const std::string &name, bool optional) {
    if (!req.has_param(name)) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    auto value = req.get_param_value(name);
    if (value == "true")
        return true;
    else if (value == "false")
        return false;
    else
    {
        error(res, "Parameter has invalid value: " + name + " = " + value);
        return {};
    }
}

template <>
inline std::optional<int> param<int>(const httplib::Request &req, httplib::Response &res, const std::string &name,
                                     bool optional) {
    if (!req.has_param(name)) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return std::stoi(req.get_param_value(name));
}

template <>
inline std::optional<Charge::Algorithm>
param<Charge::Algorithm>(const httplib::Request &req, httplib::Response &res, const std::string &name,
                         bool optional) {
    if (!req.has_param(name)) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    auto value = req.get_param_value(name);
    if (value == "fastest_bi_dijkstra") {
        return Charge::Algorithm::FASTEST_BI_DIJKSTRA;
    } else if (value == "mc_dijkstra") {
        return Charge::Algorithm::MC_DIJKSTRA;
    } else if (value == "mcc_dijkstra") {
        return Charge::Algorithm::MCC_DIJKSTRA;
    } else if (value == "fp_dijkstra") {
        return Charge::Algorithm::FP_DIJKSTRA;
    } else if (value == "fpc_dijkstra") {
        return Charge::Algorithm::FPC_DIJKSTRA;
    } else if (value == "fpc_profile_dijkstra") {
        return Charge::Algorithm::FPC_PROFILE_DIJKSTRA;
    }

    error(res, "Unknown algorithm: " + value);
    return {};
}

template <>
inline std::optional<double> param<double>(const httplib::Request &req, httplib::Response &res,
                                           const std::string &name, bool optional) {
    if (!req.has_param(name)) {
        if (!optional)
            error(res, "Parameter not found: " + name);
        return {};
    }

    return std::stof(req.get_param_value(name));
}

inline void handle_route(const Charge &charge, const httplib::Request &req, httplib::Response &res) {
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
        std::ostringstream ss;
        ss << to_json(*start, *target, routes);
        res.set_content(ss.str(), "application/json");
    }
}

inline void handle_nearest(const Charge &charge, const httplib::Request &req, httplib::Response &res) {
    auto lat = param<double>(req, res, "lat");
    auto lon = param<double>(req, res, "lon");
    if (!lat || !lon)
        return;

    auto nearest = charge.nearest(common::Coordinate::from_floating(*lon, *lat));
    std::ostringstream ss;
    ss << to_json(nearest);
    res.set_content(ss.str(), "application/json");
}
} // namespace detail

class HTTPServer {
  public:
    HTTPServer(const Charge &charge, unsigned port) {
        server.Get("/route", [&](const httplib::Request &req, httplib::Response &res) {
            detail::handle_route(charge, req, res);
        });
        server.Get("/nearest", [&](const httplib::Request &req, httplib::Response &res) {
            detail::handle_nearest(charge, req, res);
        });

        worker_thread = std::thread([this, port]() {
            server.listen("0.0.0.0", port);
        });
    }

    ~HTTPServer() {
      stop();
    }

    void stop() {
        server.stop();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    void wait() { worker_thread.join(); }

  private:
    httplib::Server server;
    std::thread worker_thread;
};
} // namespace charge::server

#endif
