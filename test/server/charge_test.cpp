#include "server/charge.hpp"

#include "common/files.hpp"
#include "ev/files.hpp"

#include "../helper/function_printer.hpp"

#include "ev/limited_tradeoff_function.hpp"

#include <catch.hpp>

#include <algorithm>
#include <vector>

using namespace charge;
using namespace charge::server;

TEST_CASE("Start server", "[Server]") {
    const std::string base = TEST_DIR "/data/charge_test";
    {
        //                        8
        //                        |
        //   0-----1-----2----4---7---9
        //    \   /      |    |   |
        //     \ /       |    |   10
        //      3        5----6
        common::WeightedGraph<ev::LimitedTradeoffFunction> graph{
            11, std::vector<common::WeightedGraph<ev::LimitedTradeoffFunction>::edge_t>{
                    {0, 1, ev::make_constant(0.1, 1)},  {0, 3, ev::make_constant(0.1, 1)},
                    {1, 0, ev::make_constant(0.2, 1)},  {1, 2, ev::make_constant(0.2, 1)},
                    {1, 3, ev::make_constant(0.2, 1)},  {2, 1, ev::make_constant(0.3, 1)},
                    {2, 4, ev::make_constant(0.3, 1)},  {2, 5, ev::make_constant(0.3, 1)},
                    {3, 0, ev::make_constant(0.4, 1)},  {3, 1, ev::make_constant(0.4, 1)},
                    {4, 2, ev::make_constant(0.5, 1)},  {4, 6, ev::make_constant(0.5, 1)},
                    {4, 7, ev::make_constant(0.5, 1)},  {5, 2, ev::make_constant(0.6, 1)},
                    {5, 6, ev::make_constant(2.0, 1)},  {6, 4, ev::make_constant(0.7, 1)},
                    {6, 5, ev::make_constant(0.7, 1)},  {7, 4, ev::make_constant(0.8, 1)},
                    {7, 8, ev::make_constant(0.8, 1)},  {7, 9, ev::make_constant(0.8, 1)},
                    {7, 10, ev::make_constant(0.8, 1)}, {8, 7, ev::make_constant(0.9, 1)},
                    {9, 7, ev::make_constant(1.0, 1)},  {10, 7, ev::make_constant(1.1, 1)}}};
        common::files::write_weighted_graph(base, graph);

        std::vector<common::Coordinate> coords{
            common::Coordinate::from_floating(0.0, 0.0), // 0
            common::Coordinate::from_floating(1.0, 0.0), // 1
            common::Coordinate::from_floating(2.0, 0.0), // 2
            common::Coordinate::from_floating(3.0, 0.0), // 3
            common::Coordinate::from_floating(4.0, 0.0), // 4
            common::Coordinate::from_floating(0.0, 1.0), // 5
            common::Coordinate::from_floating(1.0, 1.0), // 6
            common::Coordinate::from_floating(2.0, 1.0), // 7
            common::Coordinate::from_floating(3.0, 1.0), // 8
            common::Coordinate::from_floating(4.0, 1.0), // 9
            common::Coordinate::from_floating(5.0, 1.0), // 10
        };
        common::files::write_coordinates(base, coords);

        std::vector<std::int32_t> heights{
            0,  // 0
            1,  // 1
            2,  // 2
            3,  // 3
            2,  // 4
            1,  // 5
            0,  // 6
            -1, // 7
            -2, // 8
            0,  // 9
            3,  // 10
        };
        common::files::write_heights(base, heights);

        std::vector<double> chargers{
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        };
        ev::files::write_charger(base, chargers);
    }

    // 0 -> 9:  0-1-2-4-7-9 (1,2,3,5,8)
    // 5 -> 6:  5-2-4-6     (6,3,5)
    // 3 -> 8:  3-1-2-4-7-8 (4,2,3,5,8)
    // 3 -> 6:  3-1-2-4-6   (4,2,3,5)
    // 9 -> 5:  9-7-4-2-5   (10,8,5,3)
    // 10 -> 9: 10-5-9      (11,8)
    // 2 -> 4:  2-4         (3)
    // 2 -> 2:  2           ()
    using common::Coordinate;
    std::vector<std::tuple<Coordinate, Coordinate>> queries = {
        {Coordinate::from_floating(0.0, 0.0), Coordinate::from_floating(4.0, 1.0)}, // 0->9
        {Coordinate::from_floating(0.0, 1.0), Coordinate::from_floating(1.0, 1.0)}, // 5->6
        {Coordinate::from_floating(3.0, 0.0), Coordinate::from_floating(3.0, 1.0)}, // 3->8
        {Coordinate::from_floating(3.0, 0.0), Coordinate::from_floating(1.0, 1.0)}, // 3->6
        {Coordinate::from_floating(4.0, 1.0), Coordinate::from_floating(0.0, 1.0)}, // 9->5
        {Coordinate::from_floating(5.0, 1.0), Coordinate::from_floating(4.0, 1.0)}, // 10->9
        {Coordinate::from_floating(2.0, 0.0), Coordinate::from_floating(4.0, 0.0)}, // 2->4
        {Coordinate::from_floating(2.0, 0.0), Coordinate::from_floating(2.0, 0.0)}, // 2->2
    };

    std::vector<RouteResult> references{
        RouteResult{ev::make_constant(1.9, 5),
                    {0.0, 0.1, 0.3, 0.6, 1.1, 1.9},
                    {4004146.8000000319, 2002073.400000016, 2669431.2000000216, 1790636.1476587767, 1000884.221782684},
                    {0.0, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
                    {0.0, 111226.3000000009, 222452.6000000018, 444905.2000000036, 693604.6649526114, 916023.380904319 },
                    {0, 1, 2, 4, 7, 9},
                    {Coordinate::from_floating(0, 0), Coordinate::from_floating(1, 0),
                     Coordinate::from_floating(2, 0), Coordinate::from_floating(4, 0),
                     Coordinate::from_floating(2, 1), Coordinate::from_floating(4, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(1.4, 3),
                    {0.0f, 0.6f, 0.9f, 1.4f},
                    {0.0f, 0.0f, 0.0f},
                    {0.0, 1.0f, 2.0f, 3.0f},
                    {0.0f, 248699.46875f, 471152.0625f, 822864.4375f},
                    {5, 2, 4, 6},
                    {Coordinate::from_floating(0, 1), Coordinate::from_floating(2, 0),
                     Coordinate::from_floating(4, 0), Coordinate::from_floating(1, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(2.2, 5),
                    {0.0f, 0.4f, 0.6f, 0.9f, 1.4f, 2.2f},
                    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
                    {0.0f, 222452.59375f, 333678.90625f, 556131.5f, 804830.9375f, 916040.3125f},
                    {3, 1, 2, 4, 7, 8},
                    {Coordinate::from_floating(3, 0), Coordinate::from_floating(1, 0),
                     Coordinate::from_floating(2, 0), Coordinate::from_floating(4, 0),
                     Coordinate::from_floating(2, 1), Coordinate::from_floating(3, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(1.4, 4),
                    {0.0f, 0.4f, 0.6f, 0.9f, 1.4f},
                    {0.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 2.0f, 3.0f, 4.0f},
                    {0.0f, 222452.59375f, 333678.90625f, 556131.5f, 907843.875f},
                    {3, 1, 2, 4, 6},
                    {Coordinate::from_floating(3, 0), Coordinate::from_floating(1, 0),
                     Coordinate::from_floating(2, 0), Coordinate::from_floating(4, 0),
                     Coordinate::from_floating(1, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(2.6, 4),
                    {0.0f, 1.0f, 1.8f, 2.3f, 2.6f},
                    {0.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 2.0f, 3.0f, 4.0f},
                    {0.0f, 222418.71875f, 471118.1875f, 693570.75f, 942270.25f},
                    {9, 7, 4, 2, 5},
                    {Coordinate::from_floating(4, 1), Coordinate::from_floating(2, 1),
                     Coordinate::from_floating(4, 0), Coordinate::from_floating(2, 0),
                     Coordinate::from_floating(0, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(1.9, 2),
                    {0.0f, 1.1f, 1.9f},
                    {0.0f, 0.0f},
                    {0.0f, 1.0f, 2.0f},
                    {0.0f, 333628.0625f, 556046.8125f},
                    {10, 7, 9},
                    {Coordinate::from_floating(5, 1), Coordinate::from_floating(2, 1),
                     Coordinate::from_floating(4, 1)},
                    {},
                    {}},
        RouteResult{ev::make_constant(0.3, 1),
                    {0.0f, 0.3},
                    {0.0f},
                    {0.0f, 1.0f},
                    {0.0f, 222452.59375f},
                    {2, 4},
                    {Coordinate::from_floating(2, 0), Coordinate::from_floating(4, 0)},
                    {},
                    {}},
        RouteResult{ev::make_constant(0, 0),
                    {0.0f},
                    {},
                    {0.0f},
                    {0.0f},
                    {2},
                    {Coordinate::from_floating(2, 0)},
                    {},
                    {}}};

    const Charge charge(base, 16000.0f);

    std::vector<RouteResult> results_bi_dijkstra(queries.size());

    std::transform(queries.begin(), queries.end(), results_bi_dijkstra.begin(),
                   [&charge](const auto &query) -> RouteResult {
                       auto[start_coord, target_coord] = query;
                       auto start = charge.nearest(start_coord);
                       auto target = charge.nearest(target_coord);
                       return charge
                           .route(Charge::Algorithm::FASTEST_BI_DIJKSTRA, start.id, target.id)
                           .front();
                   });

    for (auto index : common::irange<std::size_t>(0, references.size())) {
        auto &result = results_bi_dijkstra[index];
        auto &ref = references[index];
        CHECK(result.lengths == ref.lengths);
        CHECK(result.durations == ref.durations);
        CHECK(result.consumptions == ref.consumptions);
        CHECK(result.geometry == ref.geometry);
        CHECK(result.path == ref.path);
        CHECK(result.max_speeds == ref.max_speeds);
    }

    std::vector<RouteResult> results_mc_dijkstra(queries.size());

    std::transform(
        queries.begin(), queries.end(), results_mc_dijkstra.begin(),
        [&charge](const auto &query) -> RouteResult {
            auto[start_coord, target_coord] = query;
            auto start = charge.nearest(start_coord);
            auto target = charge.nearest(target_coord);
            // FIXME we only test the first route here
            return charge.route(Charge::Algorithm::MC_DIJKSTRA, start.id, target.id).front();
        });

    for (auto index : common::irange<std::size_t>(0, references.size())) {
        auto &result = results_mc_dijkstra[index];
        auto &ref = references[index];
        CHECK(result.lengths == ref.lengths);
        CHECK(result.durations == ref.durations);
        CHECK(result.consumptions == ref.consumptions);
        CHECK(result.geometry == ref.geometry);
        CHECK(result.path == ref.path);
        CHECK(result.max_speeds == ref.max_speeds);
    }
}
