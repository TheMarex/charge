#ifndef CHARGE_EXPERIMENTS_RESULT_LOGGER
#define CHARGE_EXPERIMENTS_RESULT_LOGGER

#include "ev/dijkstra.hpp"

#include "server/to_json.hpp"
#include "server/to_result.hpp"

#include <fstream>
#include <mutex>
#include <thread>

namespace charge::experiments {
class ResultLogger {
  public:
    ResultLogger(const std::string &path, const std::vector<common::Coordinate> &coordinates,
                 const std::vector<std::int32_t> &heights)
        : out(path), coordinates(coordinates), heights(heights) {}

    ResultLogger(const ResultLogger&) = delete;
    ResultLogger&operator=(const ResultLogger&) = delete;

    template <typename QueryT, typename SolutionT, typename ContextT>
    void log(const QueryT &query, const std::vector<SolutionT> &solutions,
             const ContextT &context) const {
        std::lock_guard<std::mutex> guard{output_mutex};

        if (solutions.empty()) {
            out << "null" << std::endl;
        } else {
            auto route =
                server::to_result(query.start, query.target, solutions.front(), context.labels);
            annotate_heights(route, heights);
            annotate_coordinates(route, coordinates);
            annotate_lengths(route);

            out << server::to_json(query.start, query.target, {route}) << std::endl;
        }
    }

    void flush() const {
        std::lock_guard<std::mutex> guard{output_mutex};
        out.flush();
    }

    template <typename QueryT, typename GraphT>
    void log(const QueryT &query, const int solution,
             const ev::DijkstraContext<GraphT> &context) const {
        std::lock_guard<std::mutex> guard{output_mutex};

        if (solution == common::INF_WEIGHT) {
            out << "null" << std::endl;
        } else {
            auto route = server::to_result(query.start, query.target, context.tradeoff_graph, context.costs, context.parents);
            annotate_heights(route, heights);
            annotate_coordinates(route, coordinates);
            annotate_lengths(route);

            out << server::to_json(query.start, query.target, {route}) << std::endl;
        }
    }

  private:
    mutable std::mutex output_mutex;
    mutable std::ofstream out;

    const std::vector<common::Coordinate> &coordinates;
    const std::vector<std::int32_t> &heights;
};
}

#endif
