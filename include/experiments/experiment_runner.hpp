#ifndef CHARGE_RANDOM_QUERIES_HPP
#define CHARGE_RANDOM_QUERIES_HPP

#include "experiments/experiments.hpp"
#include "experiments/query.hpp"
#include "experiments/result_logger.hpp"

#include "server/to_json.hpp"
#include "server/to_result.hpp"

#include "common/csv.hpp"
#include "common/histrogram.hpp"
#include "common/irange.hpp"
#include "common/options.hpp"
#include "common/parallel_for.hpp"
#include "common/progress_bar.hpp"
#include "common/statistics.hpp"
#include "common/signal_handler.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <csignal>

namespace charge::experiments {

template <typename T, typename = int> struct has_labels : std::false_type {};

template <typename T> struct has_labels<T, decltype((void)T::labels, 0)> : std::true_type {};

template <typename ContextT> class ExperimentRunner {
  public:
    using duration_t = std::uint32_t;

    ExperimentRunner(ContextT example_context_, std::vector<Query> queries_,
                     const std::string &path, const ResultLogger &result_logger,
                     std::size_t num_runs)
        : example_context(std::move(example_context_)), example_contexts(example_context),
          queries(std::move(queries_)), path(path), result_logger(result_logger),
          num_runs(num_runs), timings(queries.size()), event_counters(queries.size()) {
              common::SignalHandler::get().set(SIGINT, [this]() {
                        dump_log(queries);
                        this->result_logger.flush();
                        std::cout << "Saved experiment state to disk. Good to abort." << std::endl;
                      });
          }

    void dump_log(const std::vector<Query> &queries) const {
        std::lock_guard<std::mutex> output_lock{output_mutex};

        common::CSVWriter<int, int, double, double, int, int, std::vector<std::size_t>> out(path);

        std::vector<std::string> headers = {"start", "target", "min_total_consumption", "max_total_consumption", "rank", "avg_time"};

        for (common::StatisticsEvent event = common::StatisticsEvent{0};
             event < common::StatisticsEvent::NUM;
             event = static_cast<common::StatisticsEvent>(static_cast<std::uint8_t>(event) + 1)) {
            headers.push_back(event_to_name(event));
        }

        out.write_header(headers);

        for (const auto &query : queries) {
            const auto &counters = event_counters[query.id];
            out.write(std::make_tuple(query.start, query.target, query.min_consumption,
                        query.max_consumption, query.rank,
                                      timings[query.id],
                                      std::vector<std::size_t>{counters.begin(), counters.end()}));
        }
    }

    ~ExperimentRunner() { dump_log(queries); }

    void run(std::size_t num_threads = 1, double max_time = std::numeric_limits<double>::infinity()) { internal_run(num_threads, max_time); }

    void summary() const {
        std::cerr << "Timings:" << std::endl;
        std::cerr << common::measurements_to_historgram(timings);

        for (common::StatisticsEvent event = common::StatisticsEvent{0};
             event < common::StatisticsEvent::NUM;
             event = static_cast<common::StatisticsEvent>(static_cast<std::uint8_t>(event) + 1)) {
            std::cerr << common::event_to_name(event) << ":" << std::endl;
            std::cerr << common::measurements_to_historgram(get_event_counts(event));
        }
    }

  private:
    void internal_run(std::size_t num_threads, const double max_time) {
        for (auto run : common::irange<std::size_t>(0, num_runs)) {
            common::ProgressBar progress(queries.size());

            const auto range = common::irange<std::size_t>(0, queries.size());
            auto query_range = [&](const auto &range) {
                ContextT context = example_context;
                for (auto idx : range) {
                    const auto &query = queries[idx];
                    progress.update();

                    if
                        constexpr(has_labels<ContextT>::value) { context.labels.clear(); context.labels.shrink_to_fit(); }

                    common::Statistics::get().reset();
                    if (common::Options::get().tail_experiment)
                        std::cerr << "Starting " << query.start << "->" << query.target << "..."
                                  << std::endl;
                    auto start = std::chrono::high_resolution_clock::now();

                    auto solutions = context(query.start, query.target);

                    auto diff = std::chrono::high_resolution_clock::now() - start;
                    auto time_us =
                        std::chrono::duration_cast<std::chrono::microseconds>(diff).count();

                    result_logger.log(query, solutions, context);

                    if (common::Options::get().tail_experiment)
                        std::cerr << "Finished " << query.start << "->" << query.target << " in " << time_us/1000. << "ms."<< std::endl;

                    if (common::Statistics::get().is_tailing())
                        std::cerr << common::Statistics::get().summary() << std::endl;

                    {
                        std::lock_guard<std::mutex> output_lock{output_mutex};
                        const auto counters = common::Statistics::get().counters();
                        for (const auto event_id :
                             common::irange<std::size_t>(0, event_counters[query.id].size())) {
                            event_counters[query.id][event_id] += counters[event_id];
                        }
                        timings[query.id] += time_us;
                    }

                    if (time_us / 1000.0 / 1000.0 > max_time)
                    {
                        std::cerr << "\nFirst query with " << (time_us / 1000.0 / 1000.0) << " > " << max_time << ". Stopping. " << std::endl;
                        break;
                    }
                }
            };

            // clang-format off
            if (num_threads > 1) {
                common::parallel_for(range, query_range, num_threads);
            } else {
                query_range(range);
            }
        }

        // build average over all runs
        for (const auto &query : queries) {
            for (auto &c : event_counters[query.id]) {
                c /= num_runs;
            }
            timings[query.id] /= num_runs;
        }
    }

    auto get_event_counts(common::StatisticsEvent event) const {
        std::vector<std::size_t> counts(event_counters.size());
        std::transform(
            event_counters.begin(), event_counters.end(), counts.begin(),
            [event](const auto &counters) { return counters[static_cast<std::size_t>(event)]; });
        return counts;
    }


    mutable std::mutex output_mutex;
    const ResultLogger &result_logger;
    ContextT example_context;
    const ContextT &example_contexts;
    std::vector<Query> queries;
    std::string path;
    std::size_t num_runs;
    std::vector<duration_t> timings;
    std::vector<common::Statistics::counters_t> event_counters;
};

template<typename ContextT>
auto make_experiment_runner(ContextT example_context, std::vector<Query> queries,
                     const std::string &path, const ResultLogger &result_logger,
                     std::size_t num_runs)
{
    return ExperimentRunner<ContextT>(std::move(example_context), std::move(queries), path, result_logger, num_runs);
}

}
#endif
