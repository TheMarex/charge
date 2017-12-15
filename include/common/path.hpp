#ifndef CHARGE_COMMON_PATH_HPP
#define CHARGE_COMMON_PATH_HPP

#include "dijkstra.hpp"
#include "mc_dijkstra.hpp"

#include <tuple>
#include <vector>

namespace charge {
namespace common {

namespace detail {
template <typename GraphT, typename OutIter>
void backtrack_path(const typename GraphT::node_id_t from, const ParentVector<GraphT> &parents,
                    OutIter iter) {
    auto node = from;
    do {
        *iter++ = node;
        node = parents[node];
    } while (node != INVALID_ID);
}

template <typename LabelEntryT, typename NodeLabelsT, typename OutIter>
void backtrack_path(const LabelEntryT from, const NodeLabelsT &labels, OutIter iter) {
    auto current = from;
    while (current.parent != INVALID_ID) {
        *iter++ = current.parent;
        current = labels[current.parent][current.parent_entry];
    }
}

template <typename LabelEntryT, typename NodeLabelsT, typename PathOutIter, typename LabelOutIter>
void backtrack_path_with_labels(const LabelEntryT from, const NodeLabelsT &labels,
                                PathOutIter path_iter, LabelOutIter label_iter) {
    auto current = from;
    while (current.parent != INVALID_ID) {
        *path_iter++ = current.parent;
        *label_iter++ = current;
        current = labels[current.parent][current.parent_entry];
    }
}
}

template <typename GraphT>
auto get_path(const typename GraphT::node_id_t start, const typename GraphT::node_id_t middle,
              const typename GraphT::node_id_t target, const ParentVector<GraphT> &forward_parents,
              const ParentVector<GraphT> &reverse_parents) {
    std::vector<typename GraphT::node_id_t> path;
    detail::backtrack_path<GraphT>(middle, forward_parents, std::back_inserter(path));
    std::reverse(path.begin(), path.end());
    // remove middle node
    assert(path.size() > 0);
    path.pop_back();
    detail::backtrack_path<GraphT>(middle, reverse_parents, std::back_inserter(path));

    (void)start;
    (void)target;
    assert(path.front() == start);
    assert(path.back() == target);

    return path;
}

template <typename LabelEntryT, typename NodeLabelsT>
auto get_path(const typename LabelEntryT::node_id_t start,
              const typename LabelEntryT::node_id_t target, const LabelEntryT target_label,
              const NodeLabelsT &labels) {
    std::vector<typename LabelEntryT::node_id_t> path;
    path.push_back(target);
    detail::backtrack_path<LabelEntryT>(target_label, labels, std::back_inserter(path));
    std::reverse(path.begin(), path.end());

    (void)start;
    (void)target;
    assert(path.front() == start);
    assert(path.back() == target);

    return path;
}

template <typename LabelEntryT, typename NodeLabelsT>
auto get_path_with_labels(const typename LabelEntryT::node_id_t start,
                          const typename LabelEntryT::node_id_t target,
                          const LabelEntryT target_label, const NodeLabelsT &labels) {
    std::vector<typename LabelEntryT::node_id_t> path;
    std::vector<LabelEntryT> path_labels;
    path.push_back(target);
    detail::backtrack_path_with_labels<LabelEntryT>(target_label, labels, std::back_inserter(path),
                                                    std::back_inserter(path_labels));
    std::reverse(path.begin(), path.end());

    assert(labels[start].size() == 1);
    path_labels.push_back(labels[start].front());
    std::reverse(path_labels.begin(), path_labels.end());

    assert(path.front() == start);
    assert(path.back() == target);
    assert(path_labels.size() == path.size());

    return std::make_tuple(std::move(path), std::move(path_labels));
}

template <typename GraphT>
auto get_path_with_labels(const typename GraphT::node_id_t start,
                          const typename GraphT::node_id_t middle,
                          const typename GraphT::node_id_t target,
                          const ParentVector<GraphT> &forward_parents,
                          const ParentVector<GraphT> &reverse_parents,
                          const CostVector<GraphT> &forward_costs,
                          const CostVector<GraphT> &reverse_costs) {
    std::vector<typename CostVector<GraphT>::value_t> path_labels;
    std::vector<typename GraphT::node_id_t> path;
    detail::backtrack_path<GraphT>(middle, forward_parents, std::back_inserter(path));
    std::reverse(path.begin(), path.end());

    std::transform(path.begin(), path.end(), std::back_inserter(path_labels),
                   [&forward_costs](const auto node) { return forward_costs[node]; });

    // remove middle node
    assert(path.size() > 0);
    path.pop_back();
    auto middle_index = path.size();
    assert(path_labels.size() == path.size() + 1);
    detail::backtrack_path<GraphT>(middle, reverse_parents, std::back_inserter(path));

    for (auto index : irange(middle_index, path.size() - 1)) {
        auto node = path[index];
        auto next_node = path[index + 1];
        auto diff = reverse_costs[node] - reverse_costs[next_node];
        path_labels.push_back(path_labels[index] + diff);
    }

    (void)start;
    (void)target;
    assert(path.front() == start);
    assert(path.back() == target);
    assert(path_labels.size() == path.size());

    return std::make_tuple(std::move(path), std::move(path_labels));
}

template <typename GraphT>
auto get_path_with_labels(const typename GraphT::node_id_t start,
                          const typename GraphT::node_id_t target,
                          const ParentVector<GraphT> &parents, const CostVector<GraphT> &costs) {
    std::vector<typename CostVector<GraphT>::value_t> path_labels;
    std::vector<typename GraphT::node_id_t> path;
    detail::backtrack_path<GraphT>(target, parents, std::back_inserter(path));
    std::reverse(path.begin(), path.end());

    std::transform(path.begin(), path.end(), std::back_inserter(path_labels),
                   [&costs](const auto node) { return costs[node]; });

    (void)start;
    (void)target;
    assert(path.front() == start);
    assert(path.back() == target);
    assert(path_labels.size() == path.size());

    return std::make_tuple(std::move(path), std::move(path_labels));
}
}
}

#endif
