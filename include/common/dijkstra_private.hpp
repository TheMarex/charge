#ifndef CHARGE_COMMON_DIJKSTRA_PRIVATE_HPP
#define CHARGE_COMMON_DIJKSTRA_PRIVATE_HPP

#include "common/id_queue.hpp"
#include "common/node_label_container.hpp"

namespace charge::common::detail {

template <typename T, typename = int> struct has_node_weights : std::false_type {};

template <typename T> struct has_node_weights<T, decltype((void)T::node_weight_penalty, 0)> : std::true_type {};

template <typename PolicyT, typename NodePotentialsT>
void insert_label(typename PolicyT::queue_t &queue, NodeLabels<PolicyT> &labels,
                  const PolicyT &policy, const NodePotentialsT &potentials,
                  const typename PolicyT::node_id_t target, typename PolicyT::label_t label) {
    auto min_changed = labels.push(target, std::move(label), policy, potentials);
    if (labels.empty(target)) {
        // pushing a new element can lead to the current min
        // being removed, since the dominance is re-checked.
        // In that case we do nothing, because don't have a random
        // access remove operation. We just wait for it to be pop'ed.
        // assert(!queue.contains_id(target));
    } else if (min_changed) {
        const auto target_key = labels.min(target).key;
        assert(target_key > 0);

        if (queue.contains_id(target)) {
            const auto current_target_key = queue.get_key(target);

            if (target_key < current_target_key) {
                queue.decrease_key(IDKeyPair{target, target_key});
            } else if (target_key > current_target_key) {
                queue.increase_key(IDKeyPair{target, target_key});
            }
        } else {
            queue.push(IDKeyPair{target, target_key});
        }
    }
}
}

#endif
