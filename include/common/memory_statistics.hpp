#ifndef CHARGE_COMMON_MEMORY_STATISTICS_HPP
#define CHARGE_COMMON_MEMORY_STATISTICS_HPP

#include "common/limited_functions_aliases.hpp"
#include "common/node_label_container.hpp"
#include "common/piecewise_functions_aliases.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace charge::common {
template <typename DataT> std::string memory_statistics(const DataT &) {
    return "No memory statistics";
}

template <typename PolicyT>
std::string memory_statistics(
    const NodeLabels<PolicyT> &labels,
    typename std::enable_if<std::is_same_v<typename NodeLabels<PolicyT>::label_t::cost_t,
                                           PiecewieseDecHypOrLinFunction>>::type * = 0) {
    using label_t = typename NodeLabels<PolicyT>::label_t;
    using cost_t = PiecewieseDecHypOrLinFunction;
    static_assert(std::is_same_v<cost_t, typename label_t::cost_t>);

    std::size_t unsettled_container_size =
        labels.unsettled_labels.size() *
        sizeof(typename decltype(labels.unsettled_labels)::value_type);
    std::size_t settled_container_size =
        labels.settled_labels.size() * sizeof(typename decltype(labels.settled_labels)::value_type);

    std::size_t unsettled_labels_capacity = 0;
    std::size_t unsettled_labels_size = 0;
    for (const auto &unsettled : labels.unsettled_labels) {
        unsettled_labels_capacity += unsettled.capacity() * sizeof(label_t);
        unsettled_labels_size += unsettled.size() * sizeof(label_t);
    }

    std::size_t settled_labels_capacity = 0;
    std::size_t settled_labels_size = 0;
    for (const auto &settled : labels.settled_labels) {
        settled_labels_capacity += settled.capacity() * sizeof(label_t);
        settled_labels_size += settled.size() * sizeof(label_t);
    }

    std::size_t unsettled_functions_size = 0;
    std::size_t unsettled_functions_capacity = 0;
    for (const auto &unsettled : labels.unsettled_labels) {
        for (const auto &label : unsettled) {
            unsettled_functions_size += label.cost.size() * sizeof(LimitedHypOrLinFunction);
            unsettled_functions_capacity += label.cost.capacity() * sizeof(LimitedHypOrLinFunction);
        }
    }
    std::size_t settled_functions_size = 0;
    std::size_t settled_functions_capacity = 0;
    for (const auto &settled : labels.settled_labels) {
        for (const auto &label : settled) {
            settled_functions_size += label.cost.size() * sizeof(LimitedHypOrLinFunction);
            settled_functions_capacity += label.cost.capacity() * sizeof(LimitedHypOrLinFunction);
        }
    }

    std::size_t settled_functions_delta_size = 0;
    std::size_t settled_functions_delta_capacity = 0;
    if
        constexpr(is_delta_label<label_t>::value) {
            for (const auto &settled : labels.settled_labels) {
                for (const auto &label : settled) {
                    settled_functions_delta_size += label.delta.size() * sizeof(double) * 2;
                    settled_functions_delta_capacity += label.delta.capacity() * sizeof(double) * 2;
                }
            }
        }

    std::size_t unsettled_functions_delta_size = 0;
    std::size_t unsettled_functions_delta_capacity = 0;
    if
        constexpr(is_delta_label<label_t>::value) {
            for (const auto &unsettled : labels.unsettled_labels) {
                for (const auto &label : unsettled) {
                    unsettled_functions_delta_size += label.delta.size() * sizeof(double) * 2;
                    unsettled_functions_delta_capacity +=
                        label.delta.capacity() * sizeof(double) * 2;
                }
            }
        }

    constexpr double MB = 1 / 1024. / 1024.;

    std::stringstream ss;
    ss << "labels.unsettled_labels.size : " << unsettled_container_size * MB << std::endl;
    ss << "labels.settled_labels.size : " << settled_container_size * MB << std::endl;

    ss << "labels.unsettled_labels[i].capacity : " << unsettled_labels_capacity * MB << std::endl;
    ss << "labels.unsettled_labels[i].size : " << unsettled_labels_size * MB << std::endl;
    ss << "labels.settled_labels[i].capacity : " << settled_labels_capacity * MB << std::endl;
    ss << "labels.settled_labels[i].size : " << settled_labels_size * MB << std::endl;

    ss << "labels.unsettled_labels[i].cost.functions.capacity : "
       << unsettled_functions_capacity * MB << std::endl;
    ss << "labels.unsettled_labels[i].cost.functions.size : " << unsettled_functions_size * MB
       << std::endl;
    ss << "labels.settled_labels[i].cost.functions.capacity : " << settled_functions_capacity * MB
       << std::endl;
    ss << "labels.settled_labels[i].cost.functions.size : " << settled_functions_size * MB
       << std::endl;
    ss << "labels.unsettled_labels[i].delta.functions.capacity : "
       << unsettled_functions_delta_capacity * MB << std::endl;
    ss << "labels.unsettled_labels[i].delta.functions.size : "
       << unsettled_functions_delta_size * MB << std::endl;
    ss << "labels.settled_labels[i].delta.functions.capacity : "
       << settled_functions_delta_capacity * MB << std::endl;
    ss << "labels.settled_labels[i].delta.functions.size : " << settled_functions_delta_size * MB
       << std::endl;
    ss << "total capacity: "
       << (unsettled_container_size + settled_container_size + unsettled_labels_capacity +
           settled_labels_capacity + unsettled_functions_capacity + settled_functions_capacity +
           unsettled_functions_delta_capacity + settled_functions_delta_capacity) *
              MB
       << std::endl;
    ss << "total size: "
       << (unsettled_container_size + settled_container_size + unsettled_labels_size +
           settled_labels_size + unsettled_functions_size + settled_functions_size +
           unsettled_functions_delta_size + settled_functions_delta_size) *
              MB
       << std::endl;
    return ss.str();
}

template <typename PolicyT>
std::string memory_statistics(
    const NodeLabels<PolicyT> &labels,
    typename std::enable_if<std::is_same_v<typename NodeLabels<PolicyT>::label_t::cost_t,
                                           std::tuple<std::int32_t, std::int32_t>>>::type * = 0) {
    using label_t = typename NodeLabels<PolicyT>::label_t;
    using cost_t = std::tuple<std::int32_t, std::int32_t>;
    static_assert(std::is_same_v<cost_t, typename label_t::cost_t>);

    std::size_t unsettled_container_size =
        labels.unsettled_labels.size() *
        sizeof(typename decltype(labels.unsettled_labels)::value_type);
    std::size_t settled_container_size =
        labels.settled_labels.size() * sizeof(typename decltype(labels.settled_labels)::value_type);

    std::size_t unsettled_labels_capacity = 0;
    std::size_t unsettled_labels_size = 0;
    for (const auto &unsettled : labels.unsettled_labels) {
        unsettled_labels_capacity += unsettled.capacity() * sizeof(label_t);
        unsettled_labels_size += unsettled.size() * sizeof(label_t);
    }

    std::size_t settled_labels_capacity = 0;
    std::size_t settled_labels_size = 0;
    for (const auto &settled : labels.settled_labels) {
        settled_labels_capacity += settled.capacity() * sizeof(label_t);
        settled_labels_size += settled.size() * sizeof(label_t);
    }

    constexpr double MB = 1 / 1024. / 1024.;

    std::stringstream ss;
    ss << "labels.unsettled_labels.size : " << unsettled_container_size * MB << std::endl;
    ss << "labels.settled_labels.size : " << settled_container_size * MB << std::endl;

    ss << "labels.unsettled_labels[i].capacity : " << unsettled_labels_capacity * MB << std::endl;
    ss << "labels.unsettled_labels[i].size : " << unsettled_labels_size * MB << std::endl;
    ss << "labels.settled_labels[i].capacity : " << settled_labels_capacity * MB << std::endl;
    ss << "labels.settled_labels[i].size : " << settled_labels_size * MB << std::endl;

    ss << "total capacity: "
       << (unsettled_container_size + settled_container_size + unsettled_labels_capacity +
           settled_labels_capacity) *
              MB
       << std::endl;
    ss << "total size: "
       << (unsettled_container_size + settled_container_size + unsettled_labels_size +
           settled_labels_size) *
              MB
       << std::endl;
    return ss.str();
}
}

#endif
