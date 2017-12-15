#ifndef CHARGE_TEST_LABEL_PRINTER_HPP
#define CHARGE_TEST_LABEL_PRINTER_HPP

#include "common/fp_dijkstra.hpp"
#include "common/mc_dijkstra.hpp"
#include "common/node_label_container.hpp"
#include "function_printer.hpp"

namespace Catch {
template <>
std::string toString<charge::common::LabelEntry<charge::common::HypLinGraph::weight_t,
                                                charge::common::HypLinGraph::node_id_t>>(
    const charge::common::LabelEntry<charge::common::HypLinGraph::weight_t,
                                     charge::common::HypLinGraph::node_id_t> &label) {
    std::stringstream ss;
    ss << "{" << toString(label.cost) << "}";
    return ss.str();
}

template <>
std::string toString<charge::common::LabelEntryWithParent<charge::common::HypLinGraph::weight_t,
                                                          charge::common::HypLinGraph::node_id_t>>(
    const charge::common::LabelEntryWithParent<charge::common::HypLinGraph::weight_t,
                                               charge::common::HypLinGraph::node_id_t> &label) {
    std::stringstream ss;
    ss << "{ cost = " << toString(label.cost) << " delta = " << toString(label.delta)
       << " parent = " << label.parent << " entry = " << label.parent_entry << "}";
    return ss.str();
}
}

#endif
