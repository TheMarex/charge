#ifndef CHARGE_COMMON_QUEUE_STATISTICS_HPP
#define CHARGE_COMMON_QUEUE_STATISTICS_HPP

#include "common/irange.hpp"

#include <tbb/enumerable_thread_specific.h>

#include <array>
#include <atomic>
#include <string>

#include <sstream>

namespace charge::common {

enum class StatisticsEvent : std::uint8_t {
    QUEUE_PUSH = 0,
    QUEUE_POP,
    QUEUE_INCREASE_KEY,
    QUEUE_DECREASE_KEY,
    LABEL_PUSH,
    LABEL_POP,
    LABEL_MAX_NUM_UNSETTLED,
    LABEL_MAX_NUM_SETTLED,
    LABEL_MAX_LENGTH,
    LABEL_SUM_LENGTH,
    LABEL_DELTA_MAX_LENGTH,
    LABEL_DELTA_SUM_LENGTH,
    LABEL_CLEANUP,
    DIJKSTRA_STALL,
    DIJKSTRA_RELAX,
    DIJKSTRA_PARENT_PRUNE,
    DIJKSTRA_CONTRAINT_CLIP,
    DIJKSTRA_NODE_WEIGHT,
    DOMINATION,
    INTERSECTION,
    NUM,
};

inline const char *event_to_name(StatisticsEvent name) {
    static std::array<const char *, static_cast<std::size_t>(StatisticsEvent::NUM)> names{
        {"QUEUE_PUSH",
         "QUEUE_POP",
         "QUEUE_INCREASE_KEY",
         "QUEUE_DECREASE_KEY",
         "LABEL_PUSH",
         "LABEL_POP",
         "LABEL_MAX_NUM_UNSETTLED",
         "LABEL_MAX_NUM_SETTLED",
         "LABEL_MAX_LENGTH",
         "LABEL_SUM_LENGTH",
         "LABEL_DELTA_MAX_LENGTH",
         "LABEL_DELTA_SUM_LENGTH",
         "LABEL_CLEANUP",
         "DIJKSTRA_STALL",
         "DIJKSTRA_RELAX",
         "DIJKSTRA_PARENT_PRUNE",
         "DIJKSTRA_CONTRAINT_CLIP",
         "DIJKSTRA_NODE_WEIGHT",
         "DOMINATION",
         "INTERSECTION"}};
    return names[static_cast<std::size_t>(name)];
}

#ifdef CHARGE_ENABLE_STATISTICS
class Statistics {
    Statistics() noexcept;

  public:
    using counters_t = std::array<std::size_t, static_cast<std::size_t>(StatisticsEvent::NUM)>;

    static Statistics &get();

    void count(StatisticsEvent event) {
        if (enabled) {
            event_counters[static_cast<std::size_t>(event)]++;
        }
    }

    void max(StatisticsEvent event, std::size_t value) {
        if (enabled) {
            event_counters[static_cast<std::size_t>(event)] =
                std::max(event_counters[static_cast<std::size_t>(event)], value);
        }
    }

    void sum(StatisticsEvent event, std::size_t value) {
        if (enabled) {
            event_counters[static_cast<std::size_t>(event)] += value;
        }
    }

    void disable() { enabled = false; }
    void enable() { enabled = true; }

    bool is_tailing() const { return tail; }

    // Caller needs to make sure this function is not called
    // in parallel/while couting
    void reset() {
        for (auto &x : event_counters) {
            x = 0;
        }
    }

    auto counters() /*const*/ { return event_counters; }

    std::string summary() const {
        std::stringstream ss;
        ss << summary(event_counters);
        return ss.str();
    }

    static std::string summary(const counters_t &event_counters) {
        std::stringstream ss;
        for (const auto event_id : irange<std::uint8_t>(0, event_counters.size())) {
            StatisticsEvent event{event_id};
            ss << event_to_name(event) << ": " << event_counters[event_id];
            if (event == StatisticsEvent::LABEL_SUM_LENGTH)
                ss << "("
                   << event_counters[event_id] / (double)(event_counters[static_cast<std::size_t>(
                                                              StatisticsEvent::LABEL_PUSH)] +
                                                          1)
                   << ")";
            else if (event == StatisticsEvent::LABEL_DELTA_SUM_LENGTH)
                ss << "("
                   << event_counters[event_id] / (double)(event_counters[static_cast<std::size_t>(
                                                              StatisticsEvent::LABEL_PUSH)] +
                                                          1)
                   << ")";
            ss << std::endl;
        }
        return ss.str();
    }

  private:
    bool enabled = true;
    bool tail = false;
    counters_t event_counters;
};
#else
class Statistics {
    Statistics() noexcept;

  public:
    using counters_t = std::array<std::size_t, static_cast<std::size_t>(StatisticsEvent::NUM)>;

    static Statistics &get();

    void count(StatisticsEvent) const {}

    void max(StatisticsEvent, std::size_t) const {}

    void sum(StatisticsEvent, std::size_t) const {}

    auto counters() const {
        counters_t result;
        for (auto &x : result) {
            x = 0;
        }
        return result;
    }

    void reset() const {}

    std::string summary() const { return {}; }

    static std::string summary(const counters_t &) { return {}; }

    void disable() const {}
    void enable() const {}
    bool is_tailing() const { return false; }

  private:
    const bool enabled = false;
    const bool tail = false;
};
#endif
}

#endif
