#include "preprocessing/contractor.hpp"

#include "common/graph_transform.hpp"
#include "common/dyn_graph.hpp"

#include <vector>
#include <catch.hpp>

using namespace charge;
using namespace charge::preprocessing;

TEST_CASE("test contraction with order", "[contractor]")
{
    // 1------2
    // | \   /|
    // |   0  |<-- faster then shortcut
    // | /   \|
    // 3      4
    common::DynGraph forward_graph {
        5,
        {
            {0, 1, 1},
            {0, 2, 1},
            {0, 3, 1},
            {0, 4, 1},
            {1, 0, 1},
            {1, 2, 3},
            {1, 3, 3},
            {2, 0, 1},
            {2, 1, 3},
            {2, 4, 1},
            {3, 0, 1},
            {3, 1, 3},
            {4, 0, 1},
            {4, 2, 1}
        }
    };
    common::DynGraph reverse_graph = invert(forward_graph);

    std::vector<common::DynGraph::node_id_t> order { 0, 1, 2, 3, 4 };
    std::vector<unsigned> rank { 0, 1, 2, 3, 4 };
    contract(forward_graph, reverse_graph, order, rank);

    // 1------2
    // | \   /|
    // |   0  |<-- faster then shortcut
    // | /   \|
    // 3      4
    // insert: 1->3 2, 3->1 2, 1->2 2, 2->1 2, 2->3 2, 3->2 2, 1->4 2, 4->1 2
    // ========
    // 1------2
    // | \   /|
    // |   x  |
    // | /   \|
    // 3      4
    // insert: -
    // ========
    //        2
    //       /|
    //     /  |
    //   /    |
    // 3      4
    // insert: 3->4 3, 4->3 3
    // ========
    //
    //
    //
    //
    // 3------4
    // insert: -
    const common::DynGraph reference_forward_graph {
        5,
        {
            {0, 1, 1},
            {0, 2, 1},
            {0, 3, 1},
            {0, 4, 1},
            {1, 0, 1},
            {1, 2, 2},
            {1, 3, 2},
            {1, 4, 2},
            {2, 0, 1},
            {2, 1, 2},
            {2, 3, 2},
            {2, 4, 1},
            {3, 0, 1},
            {3, 1, 2},
            {3, 2, 2},
            {3, 4, 2},
            {4, 0, 1},
            {4, 1, 2},
            {4, 2, 1},
            {4, 3, 2}
        }
    };
    const auto reference_reverse_graph = common::invert(reference_forward_graph);

    REQUIRE(forward_graph.edges() == reference_forward_graph.edges());
    REQUIRE(reverse_graph.edges() == reference_reverse_graph.edges());
}

TEST_CASE("test contraction without order", "[contractor]")
{
    // 1------2
    // | \   /|
    // |   0  |<-- faster then shortcut
    // | /   \|
    // 3      4
    ContractGraph forward_graph {
        5,
        {
            {0, 1, 1},
            {0, 2, 1},
            {0, 3, 1},
            {0, 4, 1},
            {1, 0, 1},
            {1, 2, 3},
            {1, 3, 3},
            {2, 0, 1},
            {2, 1, 3},
            {2, 4, 1},
            {3, 0, 1},
            {3, 1, 3},
            {4, 0, 1},
            {4, 2, 1}
        }
    };
    common::ContractGraph reverse_graph = invert(forward_graph);

    auto order = contract(forward_graph, reverse_graph);

    // 1------2
    // | \   /|
    // |   0  |<-- faster then shortcut
    // | /   \|
    // 3      4
    // insert: 1->3 2, 3->1 2, 1->2 2, 2->1 2, 2->3 2, 3->2 2, 1->4 2, 4->1 2
    // ========
    // 1------2
    // | \   /|
    // |   x  |
    // | /   \|
    // 3      4
    // insert: -
    // ========
    //        2
    //       /|
    //     /  |
    //   /    |
    // 3      4
    // insert: 3->4 3, 4->3 3
    // ========
    //
    //
    //
    //
    // 3------4
    // insert: -
    const common::DynGraph reference_forward_graph {
        5,
        {
            {0, 1, 1},
            {0, 2, 1},
            {0, 3, 1},
            {0, 4, 1},
            {1, 0, 1},
            {1, 2, 2},
            {1, 3, 2},
            {1, 4, 2},
            {2, 0, 1},
            {2, 1, 2},
            {2, 3, 2},
            {2, 4, 1},
            {3, 0, 1},
            {3, 1, 2},
            {3, 2, 2},
            {3, 4, 2},
            {4, 0, 1},
            {4, 1, 2},
            {4, 2, 1},
            {4, 3, 2}
        }
    };
    const auto reference_reverse_graph = common::invert(reference_forward_graph);

    REQUIRE(forward_graph.edges() == reference_forward_graph.edges());
    REQUIRE(reverse_graph.edges() == reference_reverse_graph.edges());
}

