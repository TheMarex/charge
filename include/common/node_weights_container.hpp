#ifndef CHARGE_COMMON_NODE_WEIGHTS_CONTAINER_HPP
#define CHARGE_COMMON_NODE_WEIGHTS_CONTAINER_HPP

#include <vector>

namespace charge::common {
template <typename GraphT, typename WeightT> class NodeWeights {
  public:
    using node_id_t = typename GraphT::node_id_t;
    using weight_t = WeightT;

    bool weighted(const node_id_t node) const {
        return is_weighted[static_cast<std::size_t>(node)];
    }

    weight_t weight(const node_id_t node) const { return weights[static_cast<std::size_t>(node)]; }

    std::vector<bool> is_weighted;
    std::vector<weight_t> weights;
};
}

#endif
