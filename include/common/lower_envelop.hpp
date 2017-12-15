#ifndef CHARGE_COMMON_LOWER_ENVELOP_HPP
#define CHARGE_COMMON_LOWER_ENVELOP_HPP

#include "common/adapter_iter.hpp"
#include "common/hyp_lin_function.hpp"
#include "common/intersection.hpp"
#include "common/irange.hpp"
#include "common/null_iterator.hpp"
#include "common/piecewise_function.hpp"
#include "common/statistics.hpp"

#include <queue>
#include <unordered_set>

namespace charge::common {

template <typename ValueT, typename TotalOrdering, typename PartialOrdering>
auto lower_envelop(std::vector<ValueT> values, TotalOrdering total_cmp,
                   PartialOrdering partial_cmp) {
    std::sort(values.begin(), values.end(), total_cmp);

    if (values.empty())
        return values;

    const auto *current = &values.front();
    auto new_end = std::remove_if(std::next(values.begin()), values.end(),
                                  [&current, &partial_cmp](const ValueT &value) {
                                      if (partial_cmp(*current, value)) {
                                          return true;
                                      } else {
                                          current = &value;
                                          return false;
                                      }
                                  });
    values.resize(std::distance(values.begin(), new_end));

    return values;
}

template <template <typename V, typename N> class LabelEntryT, typename T1, typename T2,
          typename NodeIDT>
auto lower_envelop(std::vector<LabelEntryT<std::tuple<T1, T2>, NodeIDT>> values) {
    return lower_envelop(std::move(values),
                         [](const auto &lhs, const auto &rhs) { return lhs.cost < rhs.cost; },
                         [](const auto &lhs, const auto &rhs) {
                             if (std::get<0>(lhs.cost) <= std::get<0>(rhs.cost)) {
                                 return std::get<1>(lhs.cost) <= std::get<1>(rhs.cost);
                             } else {
                                 return true;
                             }
                         });
}

template <typename T1, typename T2> auto lower_envelop(std::vector<std::tuple<T1, T2>> values) {
    return lower_envelop(std::move(values),
                         [](const auto &lhs, const auto &rhs) { return lhs < rhs; },
                         [](const auto &lhs, const auto &rhs) {
                             if (std::get<0>(lhs) <= std::get<0>(rhs)) {
                                 return std::get<1>(lhs) <= std::get<1>(rhs);
                             } else {
                                 return true;
                             }
                         });
}

namespace detail {
struct BeginEvent {
    double x;
    double y;
    std::uint32_t segment_index;

    bool operator==(const BeginEvent &other) const {
        return std::tie(x, y, segment_index) == std::tie(other.x, other.y, other.segment_index);
    }
};

struct EndEvent {
    double x;
    double y;
    std::uint32_t segment_index;

    bool operator==(const EndEvent &other) const {
        return std::tie(x, y, segment_index) == std::tie(other.x, other.y, other.segment_index);
    }
};

struct IntersectionEvent {
    double x;
    double y;
    std::uint32_t first_index;
    std::uint32_t second_index;

    bool operator==(const IntersectionEvent &other) const {
        return std::tie(x, y, first_index, second_index) ==
               std::tie(other.x, other.y, other.first_index, other.second_index);
    }
};

struct SweeplineEvent {
    bool is_intersection() const { return type == Type::Intersection; }

    bool is_begin() const { return type == Type::Begin; }

    bool is_end() const { return type == Type::End; }

    bool operator<(const SweeplineEvent &other) const {
        return std::tie(begin_event.x, type, begin_event.y) <
               std::tie(other.begin_event.x, other.type, other.begin_event.y);
    }

    bool operator==(const SweeplineEvent &other) const {
        if (other.type == type) {
            switch (type) {
            case Type::End:
                return end_event == other.end_event;
            case Type::Begin:
                return begin_event == other.begin_event;
            case Type::Intersection:
                return intersection_event == other.intersection_event;
            }
        }

        return false;
    }

    bool operator>(const SweeplineEvent &other) const { return !operator<(other); }

    SweeplineEvent(BeginEvent begin_event_)
        : begin_event(std::move(begin_event_)), type(Type::Begin) {
        assert(is_begin());
    }

    SweeplineEvent(EndEvent end_event_) : end_event(std::move(end_event_)), type(Type::End) {
        assert(is_end());
    }

    SweeplineEvent(IntersectionEvent intersection_event_)
        : intersection_event(std::move(intersection_event_)), type(Type::Intersection) {
        assert(is_intersection());
    }

    union {
        BeginEvent begin_event;
        EndEvent end_event;
        IntersectionEvent intersection_event;
    };

    // order matters for tie breaking
    enum class Type { End = 0, Begin = 1, Intersection = 2 } type;

    static_assert(sizeof(BeginEvent) <= sizeof(IntersectionEvent));
    static_assert(sizeof(EndEvent) <= sizeof(IntersectionEvent));
};
}

template <template <typename V, typename N> class LabelEntryT, typename NodeIDT>
auto lower_envelop(
    std::vector<LabelEntryT<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>, NodeIDT>>
        labels) {
    std::sort(labels.begin(), labels.end());
    return labels;
}
template <template <typename V, typename N> class LabelEntryT, typename NodeIDT>
auto lower_envelop(
    std::vector<LabelEntryT<PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound>, NodeIDT>>
        labels) {
    std::sort(labels.begin(), labels.end());
    return labels;
}

namespace detail {

template <typename Iter, typename OutIter, typename OutIndexIter>
inline auto lower_envelop(const Iter begin, const Iter end, OutIter out, OutIndexIter out_index) {
    constexpr double EPSILON = 0.001;
    using BeginEvent = detail::BeginEvent;
    using EndEvent = detail::EndEvent;
    using IntersectionEvent = detail::IntersectionEvent;
    std::unordered_set<std::uint32_t> active_segments;
    std::priority_queue<detail::SweeplineEvent, std::vector<detail::SweeplineEvent>,
                        std::greater<detail::SweeplineEvent>>
        events;
    auto current_x = -std::numeric_limits<double>::infinity();
    auto prev_x = -std::numeric_limits<double>::infinity();
    auto num_functions = std::distance(begin, end);

    if (num_functions == 0)
        return;

    std::uint32_t min_index = num_functions;
    std::uint32_t prev_min_index = num_functions;

    auto functions = begin;
    using Function = std::remove_reference_t<decltype(functions[0])>;

    for (const auto index : irange<std::uint32_t>(0, num_functions)) {
        const auto &function = functions[index];
        events.emplace(BeginEvent{function.min_x, function(function.min_x), index});
    }

    std::vector<bool> was_checked(num_functions * num_functions, false);

    const auto insert_intersection = [&](auto first_index, auto second_index) {
        if (was_checked[first_index * num_functions + second_index])
            return;

        const auto &first_function = functions[first_index];
        const auto &second_function = functions[second_index];
        was_checked[first_index * num_functions + second_index] = true;
        was_checked[second_index * num_functions + first_index] = true;

        Statistics::get().count(StatisticsEvent::INTERSECTION);

        std::array<double, 4> intersection_points;
        auto intersection_points_end =
            intersection(first_function, second_function, intersection_points.begin());
        std::for_each(
            intersection_points.begin(), intersection_points_end, [&](const double intersection_x) {
                // future events and events for the same x value (in case we start with an
                // intersection)
                if (intersection_x >= current_x) {
                    events.emplace(IntersectionEvent{intersection_x, first_function(intersection_x),
                                                     first_index, second_index});
                }
            });
    };

    const auto output_minimum = [&](auto min_x, auto max_x, auto index) {
        *out_index++ = index;
        const auto &function = functions[index];
        if (min_x > function.max_x) {
            *out++ = Function{min_x, max_x, ConstantFunction{function(min_x)}};
        } else {
            max_x = std::min(max_x, function.max_x);
            *out++ = Function{min_x, max_x, function.function};
        }
    };

    while (!events.empty()) {
        auto event = events.top();

        // Remove possible duplicate events
        while (!events.empty() && events.top() == event)
            events.pop();

        auto new_min_index = min_index;

        if (event.is_intersection()) {
            const auto &intersection_event = event.intersection_event;
            current_x = intersection_event.x;

            const auto first_index = intersection_event.first_index;
            const auto second_index = intersection_event.second_index;

            // check if the intersection event might be from an old min
            if (first_index == min_index) {
                auto min_y = functions[min_index](current_x + EPSILON);
                auto new_y = functions[second_index](current_x + EPSILON);
                if (new_y < min_y) {
                    new_min_index = second_index;
                }
            } else if (second_index == min_index) {
                auto min_y = functions[min_index](current_x + EPSILON);
                auto new_y = functions[first_index](current_x + EPSILON);
                if (new_y < min_y) {
                    new_min_index = first_index;
                }
            }
        } else if (event.is_begin()) {
            const auto &begin_event = event.begin_event;
            current_x = begin_event.x;

            const auto index = begin_event.segment_index;
            const auto &function = functions[index];
            active_segments.emplace(index);
            events.emplace(EndEvent{function.max_x, function(function.max_x), index});

            if (min_index < num_functions) {
                if (functions[index](current_x + EPSILON) <
                    functions[min_index](current_x + EPSILON)) {
                    new_min_index = index;
                } else {
                    insert_intersection(min_index, index);
                }
            } else {
                new_min_index = index;
            }
        } else {
            assert(event.is_end());
            const auto &end_event = event.end_event;
            current_x = end_event.x;

            active_segments.erase(end_event.segment_index);
        }

        if (min_index != new_min_index) {
            min_index = new_min_index;

            for (const auto other_index : active_segments) {
                if (other_index != min_index) {
                    insert_intersection(min_index, other_index);
                }
            }
        }

        // First check if we are finished with all events of this x coordinate
        if (events.empty() || events.top().begin_event.x > current_x + 0.00001) {
            // If yes determine if something changed for the minimum
            if (prev_min_index != min_index) {

                if (prev_x + 0.00001 < current_x)
                {
                    // don't output the first segment from -inf to min_x
                    if (prev_min_index < num_functions) {

                        output_minimum(prev_x, current_x, prev_min_index);
                    }
                    prev_min_index = min_index;
                    prev_x = current_x;
                }
            }
        }
    }
    output_minimum(prev_x, std::numeric_limits<double>::infinity(), min_index);
}
}

inline auto lower_envelop(
    const std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> &functions) {
    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> pwf;

    detail::lower_envelop(functions.begin(), functions.end(), std::back_inserter(pwf.functions),
                          NullOutputIter<std::uint32_t>{});

    return pwf;
}

inline auto lower_envelop(
    const std::vector<
        PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing>> &pwfs) {
    // FIXME this is a naive implementation
    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> functions;
    for (const auto &pwf : pwfs) {
        functions.insert(functions.end(), pwf.functions.begin(), pwf.functions.end());
    }
    return lower_envelop(functions);
}
}

#endif
