#ifndef CHARGE_COMMON_NODE_LABEL_CONTAINER_HPP
#define CHARGE_COMMON_NODE_LABEL_CONTAINER_HPP

#include "common/adapter_iter.hpp"
#include "common/constants.hpp"
#include "common/function_graph.hpp"
#include "common/interpolating_function.hpp"
#include "common/linear_function.hpp"
#include "common/piecewise_function.hpp"
#include "common/range.hpp"
#include "common/statistics.hpp"
#include "common/tuple_helper.hpp"

#include <algorithm>
#include <tuple>
#include <vector>

namespace charge::common {

namespace detail {
template <typename T> void shrink_cost(T &) {}

template <typename T, typename MinBoundT, typename MaxBoundT, typename MonoticityT>
void shrink_cost(PiecewieseFunction<T, MinBoundT, MaxBoundT, MonoticityT> &cost) {
    cost.shrink_to_fit();
}
} // namespace detail

template <typename CostT, typename NodeIDT> struct LabelEntry {
    using cost_t = CostT;
    using node_id_t = NodeIDT;

    LabelEntry(key_t key, const cost_t cost) : LabelEntry(key, cost, INVALID_ID, INVALID_ID) {}

    LabelEntry(key_t key, const cost_t cost, const node_id_t, const node_id_t)
        : key(key), cost(cost) {}
    LabelEntry() = default;
    LabelEntry(const LabelEntry &) = default;
    LabelEntry(LabelEntry &&) = default;
    LabelEntry &operator=(const LabelEntry &) = default;
    LabelEntry &operator=(LabelEntry &&) = default;

    inline bool operator<(const LabelEntry &other) const { return cost < other.cost; }

    inline bool operator==(const LabelEntry &other) const { return cost == other.cost; }

    inline bool operator!=(const LabelEntry &other) const { return !operator==(other); }

    key_t key;
    cost_t cost;
};

template <typename CostT, typename NodeIDT> struct LabelEntryWithParent {
    using cost_t = CostT;
    using node_id_t = NodeIDT;

    LabelEntryWithParent(key_t key, const cost_t cost, const node_id_t parent,
                         const node_id_t parent_entry)
        : key(key), cost(cost), parent(parent), parent_entry(parent_entry) {}
    LabelEntryWithParent() = default;
    LabelEntryWithParent(const LabelEntryWithParent &) = default;
    LabelEntryWithParent(LabelEntryWithParent &&) = default;
    LabelEntryWithParent &operator=(const LabelEntryWithParent &) = default;
    LabelEntryWithParent &operator=(LabelEntryWithParent &&) = default;

    inline bool operator<(const LabelEntryWithParent &other) const {
        return std::tie(cost, parent, parent_entry) <
               std::tie(other.cost, other.parent, other.parent_entry);
    }

    inline bool operator==(const LabelEntryWithParent &other) const {
        return std::tie(cost, parent, parent_entry) ==
               std::tie(other.cost, other.parent, other.parent_entry);
    }
    inline bool operator!=(const LabelEntryWithParent &other) const { return !operator==(other); }

    key_t key;
    cost_t cost;
    node_id_t parent;
    node_id_t parent_entry;
};

template <typename FnT>
struct LabelEntry<LimitedFunction<FnT, inf_bound, clamp_bound>,
                  typename FunctionGraph<FnT>::node_id_t> {
    using GraphT = FunctionGraph<FnT>;
    using cost_t = PiecewieseFunction<FnT, inf_bound, clamp_bound, monotone_decreasing>;
    using delta_t = InterpolatingFunction<inf_bound, clamp_bound, monotone_increasing>;
    using node_id_t = typename GraphT::node_id_t;

    LabelEntry(key_t key, cost_t cost) noexcept
        : LabelEntry(key, std::move(cost), {}, INVALID_ID, INVALID_ID) {}
    LabelEntry(key_t key, cost_t cost_, delta_t, const node_id_t, const node_id_t) noexcept
        : key(key), cost(std::move(cost_)) {}
    LabelEntry() noexcept = default;
    LabelEntry(const LabelEntry &) noexcept = default;
    LabelEntry(LabelEntry &&) noexcept = default;
    LabelEntry &operator=(const LabelEntry &) noexcept = default;
    LabelEntry &operator=(LabelEntry &&) noexcept = default;

    inline bool operator<(const LabelEntry &other) const {
        return cost.min_x() < other.cost.min_x();
    }

    inline bool operator==(const LabelEntry &other) const {
        return cost.min_x() == other.cost.min_x();
    }

    inline bool operator!=(const LabelEntry &other) const { return !operator==(other); }

    key_t key;
    cost_t cost;
};

template <typename FnT>
struct LabelEntryWithParent<LimitedFunction<FnT, inf_bound, clamp_bound>,
                            typename FunctionGraph<FnT>::node_id_t> {
    using GraphT = FunctionGraph<FnT>;
    using cost_t = PiecewieseFunction<FnT, inf_bound, clamp_bound, monotone_decreasing>;
    using delta_t = InterpolatingFunction<inf_bound, clamp_bound, monotone_increasing>;
    using node_id_t = typename GraphT::node_id_t;

    LabelEntryWithParent(key_t key, cost_t cost_, delta_t delta_, const node_id_t parent,
                         const node_id_t parent_entry)
        : key(key), cost(std::move(cost_)), delta(std::move(delta_)), parent(parent),
          parent_entry(parent_entry) {}
    LabelEntryWithParent() noexcept = default;
    LabelEntryWithParent(const LabelEntryWithParent &) noexcept = default;
    LabelEntryWithParent(LabelEntryWithParent &&) noexcept = default;
    LabelEntryWithParent &operator=(const LabelEntryWithParent &) noexcept = default;
    LabelEntryWithParent &operator=(LabelEntryWithParent &&) noexcept = default;

    inline bool operator<(const LabelEntryWithParent &other) const {
        return cost.min_x() < other.cost.min_x();
    }

    inline bool operator==(const LabelEntryWithParent &other) const {
        return (cost.min_x() == other.cost.min_x()) &&
               std::tie(parent, parent_entry) == std::tie(other.parent, other.parent_entry);
    }

    inline bool operator!=(const LabelEntryWithParent &other) const { return !operator==(other); }

    key_t key;
    cost_t cost;
    delta_t delta;
    node_id_t parent;
    node_id_t parent_entry;
};

template <typename T, typename = int> struct is_parent_label : std::false_type {};

template <typename T> struct is_parent_label<T, decltype((void)T::parent, 0)> : std::true_type {};

template <typename T, typename = int> struct is_delta_label : std::false_type {};

template <typename T> struct is_delta_label<T, decltype((void)T::delta, 0)> : std::true_type {};

// Label container for every every node.
//
// Labels for nodes are split into two lists:
//
// 1/ unsettled labels (labels that have been inserted vis push() but not removed via pop() again)
// 2/ settled labels (labels that have been removed via pop())
//
// The life-cycle of a label is:
// push -> unsettled labels -> pop -> settled labels
//
// The invariants that govern these labels sets are:
// 1. The minimal unsettled label is not dominated by any unsettled labels.
// 1. The minimal unsettled label is not dominated by any settled labels.
//
// To ensure 1. we use a sorting in the heap that only places undominated labels as min.
// To ensure 2. we need do dominance checks every time we updated the minimum (push or pop)
template <typename PolicyT> class NodeLabels {
  public:
    using label_t = typename PolicyT::label_t;
    using cost_t = typename label_t::cost_t;
    using node_id_t = typename label_t::node_id_t;

    NodeLabels(const std::size_t num_nodes)
        : unsettled_labels(num_nodes, std::vector<label_t>{}),
          settled_labels(num_nodes, std::vector<label_t>{}) {}

    void clear() {
        for (auto &labels : unsettled_labels) {
            labels.clear();
        }
        for (auto &labels : settled_labels) {
            labels.clear();
        }
    }

    void shrink_to_fit() {
        for (auto &labels : unsettled_labels) {
            labels.shrink_to_fit();
        }
        for (auto &labels : settled_labels) {
            labels.shrink_to_fit();
        }
    }

    template <typename NodePotentialsT>
    auto push(const node_id_t node, label_t label, const PolicyT &policy,
              const NodePotentialsT &potentials) {
        Statistics::get().count(StatisticsEvent::LABEL_PUSH);

        auto &unsettled = unsettled_labels[node];

        bool modified_min = true;
        if (unsettled.size() > 0) {
            auto old_key = unsettled.front().key;
            if (policy.dominates(unsettled.front().cost, label.cost)) {
                modified_min = false;
            } else {
                unsettled.push_back(std::move(label));
                std::push_heap(unsettled.begin(), unsettled.end(),
                               [&](const auto &lhs, const auto &rhs) { return rhs.key < lhs.key; });

                // we updated the minium, we need to ensure consitency of the second invariant
                auto new_key = unsettled.front().key;
                modified_min = old_key != new_key;
            }
        } else {
            unsettled.push_back(std::move(label));
        }

        if (modified_min) {
            ensure_undominated_minium(node, policy, potentials);
        }

        return modified_min;
    }

    void cleanup_unsettled(const node_id_t node) {
        // clang-format off
        if constexpr(std::is_same_v<typename label_t::cost_t, std::tuple<std::int32_t, std::int32_t>>) {
            Statistics::get().count(StatisticsEvent::LABEL_CLEANUP);

            unsettled_labels[node] = lower_envelop(std::move(unsettled_labels[node]));
            std::make_heap(unsettled_labels[node].begin(), unsettled_labels[node].end(),
                           [&](const auto &lhs, const auto &rhs) {
                               return rhs.key <
                                      lhs.key;
                           });
        }
        // clang-format on
    }

    bool dominated(const node_id_t node, const cost_t &tentative_cost,
                   const PolicyT &policy) const {
        const auto label_to_cost = [](const label_t &label) -> const cost_t & {
            return label.cost;
        };

        auto labels_begin = make_adapter_iter(settled_labels[node].begin(), label_to_cost);
        auto labels_end = make_adapter_iter(settled_labels[node].end(), label_to_cost);
        auto range = make_range(labels_begin, labels_end);

        return policy.dominates(range, tentative_cost);
    }

    template <typename NodePotentialsT>
    std::tuple<bool, bool> clip_dominated(const node_id_t node, label_t &label,
                                          const PolicyT &policy,
                                          const NodePotentialsT &potentials) const {
        const auto label_to_cost = [](const label_t &label) -> const cost_t & {
            return label.cost;
        };

        auto labels_begin = make_adapter_iter(settled_labels[node].begin(), label_to_cost);
        auto labels_end = make_adapter_iter(settled_labels[node].end(), label_to_cost);
        auto range = make_range(labels_begin, labels_end);

        auto[dominated, modified] = policy.clip_dominated(range, label.cost);
        if (modified && !dominated) {
            label.key = potentials.key(node, PolicyT::key(label.cost), label.cost);
        }
        return std::make_tuple(dominated, modified);
    }

    auto &min(const node_id_t node) const {
        assert(!unsettled_labels[node].empty());
        return unsettled_labels[node].front();
    }

    auto &min(const node_id_t node) {
        assert(!unsettled_labels[node].empty());
        return unsettled_labels[node].front();
    }

    auto empty(const node_id_t node) const { return unsettled_labels[node].empty(); }

    auto size(const node_id_t node) const { return unsettled_labels[node].size(); }

    template <typename NodePotentialsT>
    auto pop(const node_id_t node, const PolicyT &policy, const NodePotentialsT &potentials) {
        Statistics::get().count(StatisticsEvent::LABEL_POP);
        auto &unsettled = unsettled_labels[node];
        auto &settled = settled_labels[node];

        assert(!unsettled.empty());
        std::pop_heap(unsettled.begin(), unsettled.end(),
                      [&](const auto &lhs, const auto &rhs) { return rhs.key < lhs.key; });
        auto top_entry_id = settled.size();
        // assert(!dominated(node, unsettled.back().cost));
        settled.push_back(std::move(unsettled.back()));
        unsettled.pop_back();

        ensure_undominated_minium(node, policy, potentials);

        return std::tuple<const label_t &, std::size_t>{settled.back(), top_entry_id};
    }

    // Returns all _settled_ labels of the given node
    const std::vector<label_t> &operator[](const node_id_t node) const {
        return settled_labels[node];
    }

    template <typename NodePotentialsT>
    void ensure_undominated_minium(const node_id_t node, const PolicyT &policy,
                                   const NodePotentialsT &potentials) {
        auto &unsettled = unsettled_labels[node];
        bool modified_min = true;
        while (!unsettled.empty() && modified_min) {
            auto &current_min = min(node);
            auto old_key = current_min.key;
            auto[is_dominated, was_modified] =
                clip_dominated(node, current_min, policy, potentials);
            if (is_dominated) {
                modified_min = true;
            } else if (was_modified) {
                // FIXME this is not true since the order in which
                // we test the domination matters
                // assert(!dominated(node, current_min.cost));
                // clang-format off
                if constexpr(is_delta_label<label_t>::value) {
                    current_min.delta.limit_from_x(current_min.cost.min_x(),
                                                   current_min.cost.max_x());
                    current_min.delta.shrink_to_fit();
                }
                detail::shrink_cost(current_min.cost);
                // clang-format on
                auto new_key = current_min.key;
                assert(new_key+to_fixed(0.01) >= old_key);
                modified_min = new_key > old_key;
            } else {
                modified_min = false;
                assert(current_min.key == old_key);
            }

            if (modified_min) {
                std::pop_heap(unsettled.begin(), unsettled.end(),
                              [&](const auto &lhs, const auto &rhs) { return rhs.key < lhs.key; });

                // either remove or re-insert
                if (is_dominated) {
                    unsettled.pop_back();
                } else {
                    std::push_heap(
                        unsettled.begin(), unsettled.end(),
                        [&](const auto &lhs, const auto &rhs) { return rhs.key < lhs.key; });
                }
            }
        }
        // assert(unsettled.empty() || !dominated(node, min(node).cost));
    }

    std::vector<std::vector<label_t>> settled_labels;
    std::vector<std::vector<label_t>> unsettled_labels;
};
} // namespace charge::common

#endif
