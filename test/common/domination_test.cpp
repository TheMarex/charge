#include "common/domination.hpp"

#include "common/piecewise_functions_aliases.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Piecewiese domination test with one function each", "[domination]") {
    // 6 -
    //  |   x          \
    //5 -   `           \
    //  |   `            \
    //4 -    \            \
    //  |     \            \
    //3 -      \            \
    //  |        x           \
    //2 -          ` .        \
    //  |             ` -----------------------
    //1 -                       \
    //  |                        \
    //0 +---|---|---|---|---|---|---|---|---|---|--->
    //      1   2   3   4   5   6   7   8   9   10
    PiecewieseDecHypOrLinFunction lhs_1{{{1, 4, HyperbolicFunction{4, 0, 1.5}}}};
    PiecewieseDecHypOrLinFunction rhs_1{{{3, 7, LinearFunction{-2, 13}}}};

    PiecewieseDecHypOrLinFunction lhs_2{{{1, 4, HyperbolicFunction{4, 0, 1.5}}}};
    PiecewieseDecHypOrLinFunction rhs_2{{{3, 5, LinearFunction{-2, 13}}}};

    PiecewieseDecHypOrLinFunction lhs_3{{{1, 4, HyperbolicFunction{4, 0, 1.5}}}};
    PiecewieseDecHypOrLinFunction rhs_3{{{0.5, 5, LinearFunction{-2, 13}}}};

    // intersection at x=5.625
    CHECK(lhs_1(5.625) == rhs_1(5.625));
    CHECK(!dominates_limited(lhs_1.functions[0], rhs_1.functions[0]));
    REQUIRE(!dominates_piecewiese(lhs_1, rhs_1));

    CHECK(lhs_2(5) < rhs_2(5));
    CHECK(dominates_limited(lhs_2.functions[0], rhs_2.functions[0]));
    REQUIRE(dominates_piecewiese(lhs_2, rhs_2));

    CHECK(lhs_3(0.5) > rhs_3(0.5));
    CHECK(!dominates_limited(lhs_3.functions[0], rhs_3.functions[0]));
    REQUIRE(!dominates_piecewiese(lhs_3, rhs_3));
}

TEST_CASE("Piecewiese domination test with piecewise linear", "[domination]") {
    // 7 -           :\
    //   |           : \
    // 6 -           :  \
    //   |    .      :   \
    // 5 -    `      :    \
    //   |    `      :    :\
    // 4 -    :\     :    : \
    //   |    : \    :    :  \
    // 3 -    :  \        :   \
    //   |    :    .      :    `
    // 2 -    :     ` .   :    : `
    //   |    :         ` .    :   `
    // 1 -    :           : ` .      `.............
    //   |    :           :      `    :
    // 0 +---|---|---|---|---|---|---|---|---|---|--->
    //       1   2   3   4   5   6   7   8   9   10
    PiecewieseDecHypOrLinFunction lhs_1{
        {{1, 4, HyperbolicFunction{4, 0, 1.5}}, {4, 6, LinearFunction{-0.5, 4, 1.75}}}};
    PiecewieseDecHypOrLinFunction rhs_1{{{3, 4, LinearFunction{-2, 13}},
                                         {4, 5, LinearFunction{-2, 13}},
                                         {5, 7, LinearFunction{-1, 5, 3}}}};

    // intersection at x=5.625
    CHECK(lhs_1(7) <= rhs_1(7));
    REQUIRE(dominates_piecewiese(lhs_1, rhs_1));
}

TEST_CASE("Dominates constant functions", "[domination]") {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    PiecewieseDecHypOrLinFunction lhs{{{4, inf, LinearFunction{0, 0, 4}}}};
    PiecewieseDecHypOrLinFunction rhs{{{4, inf, LinearFunction{0, 0, 4}}}};

    REQUIRE(dominates_piecewiese(lhs, rhs));
}

TEST_CASE("Find first undominated with piecewise linear", "[domination]") {
    // 7 -
    //   | \
    // 6 -  \        x
    //   |   \        `
    // 5 -    \         `
    //   |     \          `
    // 4 -      \           ` x
    //   |       \              `
    // 3 -        x                `
    //   |          `                  `
    // 2 -            `                   x
    //   |              `                    ` .
    // 1 -                `                       x
    //   |                  `
    // 0 +---|---|---|---|---x---|---|---|---|---|--->
    //       1   2   3   4   5   6   7   8   9   10
    PiecewieseDecHypOrLinFunction lhs_1{
        {{0, 2, LinearFunction{-2, 0, 7}}, {2, 5, LinearFunction{-1, 2, 3}}}};
    PiecewieseDecHypOrLinFunction rhs_1{{{3, 5, LinearFunction{-1, 3, 6}},
                                         {5, 8, LinearFunction{-2 / 3., 5, 4}},
                                         {8, 10, LinearFunction{-1 / 2., 8, 2}}}};

    CHECK(dominates_triangle(lhs_1.functions.begin(), lhs_1.functions.end(),
                             rhs_1.functions.begin(),
                             rhs_1.functions.end()) == DominationState::UNCLEAR);

    auto begin_1 = find_first_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                          rhs_1.functions.begin(), rhs_1.functions.end());
    auto end_1 = find_last_undominated(lhs_1.functions.begin(), lhs_1.functions.end(), begin_1,
                                       rhs_1.functions.end());

    // fully dominated
    CHECK(std::distance(rhs_1.functions.begin(), begin_1) == 3);
    CHECK(std::distance(begin_1, end_1) == 0);

    // without first piece and second one partially, not dominanted
    PiecewieseDecHypOrLinFunction lhs_2{{{3.2, 5, LinearFunction{-1, 2, 3}}}};
    auto rhs_2 = rhs_1;

    auto begin_2 = find_first_undominated(lhs_2.functions.begin(), lhs_2.functions.end(),
                                          rhs_2.functions.begin(), rhs_2.functions.end());
    auto end_2 = find_last_undominated(lhs_2.functions.begin(), lhs_2.functions.end(), begin_2,
                                       rhs_2.functions.end());

    // first piece is not dominated but both end pieces
    CHECK(std::distance(rhs_2.functions.begin(), begin_2) == 0);
    CHECK(std::distance(begin_2, end_2) == 1);

    // only the first piece
    PiecewieseDecHypOrLinFunction lhs_3{{{0, 2, LinearFunction{-2, 0, 7}}}};
    auto rhs_3 = rhs_1;

    auto begin_3 = find_first_undominated(lhs_3.functions.begin(), lhs_3.functions.end(),
                                          rhs_3.functions.begin(), rhs_3.functions.end());
    auto end_3 = find_last_undominated(lhs_3.functions.begin(), lhs_3.functions.end(), begin_3,
                                       rhs_3.functions.end());

    // only the first segment is dominated
    CHECK(std::distance(rhs_3.functions.begin(), begin_3) == 1);
    CHECK(std::distance(begin_3, end_3) == 2);

    // the second one only partital
    PiecewieseDecHypOrLinFunction lhs_4{
        {{0, 2, LinearFunction{-2, 0, 7}}, {2, 3.5, LinearFunction{-1, 2, 3}}}};
    auto rhs_4 = rhs_1;

    auto begin_4 = find_first_undominated(lhs_4.functions.begin(), lhs_4.functions.end(),
                                          rhs_4.functions.begin(), rhs_4.functions.end());
    auto end_4 = find_last_undominated(lhs_4.functions.begin(), lhs_4.functions.end(), begin_4,
                                       rhs_4.functions.end());

    // only the last segment is not dominated
    CHECK(std::distance(rhs_4.functions.begin(), begin_4) == 2);
    CHECK(std::distance(begin_4, end_4) == 1);
}

TEST_CASE("Piecwise dominance - Regression test 1", "[domination]") {

    PiecewieseDecHypOrLinFunction lhs{
        {{214.960938, 336.913025, HyperbolicFunction{12956600.000000, 11.707323, 378.853241}},
         {336.913025, 387.620483, HyperbolicFunction{165770.109375, 260.851837, 472.710480}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{253.59375, 347.022949, HyperbolicFunction{5826000.000000, 97.878410, 389.826233}}}};

    auto begin = find_first_undominated(lhs.functions.begin(), lhs.functions.end(),
                                        rhs.functions.begin(), rhs.functions.end());
    auto end = find_last_undominated(lhs.functions.begin(), lhs.functions.end(), begin,
                                     rhs.functions.end());

    CHECK(std::distance(begin, end) == 1);
}

TEST_CASE("Find first undominated domination with constant function - Regression test 2",
          "[domination]") {
    // 7 -
    //   |
    // 6 -           x
    //   |            `
    // 5 -              `
    //   |                `
    // 4 -                  ` x         x
    //   |                      `
    // 3 -       x                 `
    //   |                             `
    // 2 -                                x
    //   |                                   ` .
    // 1 -                                        x
    //   |
    // 0 +---|---|---|---|---x---|---|---|---|---|--->
    //       1   2   3   4   5   6   7   8   9   10

    // lhs dominates rhs partially
    PiecewieseDecHypOrLinFunction lhs_1{{{2, 2, LinearFunction{0, 0, 3}}}};
    PiecewieseDecHypOrLinFunction rhs_1{{{3, 5, LinearFunction{-1, 3, 6}},
                                         {5, 8, LinearFunction{-2 / 3., 5, 4}},
                                         {8, 10, LinearFunction{-1 / 2., 8, 2}}}};

    auto begin_1 = find_first_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                          rhs_1.functions.begin(), rhs_1.functions.end());
    auto end_1 = find_last_undominated(lhs_1.functions.begin(), lhs_1.functions.end(), begin_1,
                                       rhs_1.functions.end());
    CHECK(std::distance(begin_1, end_1) == 2);

    // lhs does not dominate the constant function
    auto lhs_2 = rhs_1;
    auto rhs_2 = lhs_1;

    auto begin_2 = find_first_undominated(lhs_2.functions.begin(), lhs_2.functions.end(),
                                          rhs_2.functions.begin(), rhs_2.functions.end());
    auto end_2 = find_last_undominated(lhs_2.functions.begin(), lhs_2.functions.end(), begin_2,
                                       rhs_2.functions.end());
    CHECK(std::distance(begin_2, end_2) == 1);

    // lhs dominates the constant function
    auto lhs_3 = rhs_1;
    PiecewieseDecHypOrLinFunction rhs_3{{{8, 8, LinearFunction{0, 0, 4}}}};

    auto begin_3 = find_first_undominated(lhs_3.functions.begin(), lhs_3.functions.end(),
                                          rhs_3.functions.begin(), rhs_3.functions.end());
    auto end_3 = find_last_undominated(lhs_3.functions.begin(), lhs_3.functions.end(), begin_3,
                                       rhs_3.functions.end());
    CHECK(std::distance(begin_3, end_3) == 0);
}

TEST_CASE("Hyperbolic piecewise functions regression test - Regression 3", "[domination]") {
    PiecewieseDecHypOrLinFunction lhs{
        {{{1114.949234, 1123.161060, HyperbolicFunction{4092402.520534, 995.877771, 2766.375858}},
          {1123.161060, 1135.613805, HyperbolicFunction{5720518.212615, 980.843967, 2736.540375}},
          {1135.613805, 1486.814377, HyperbolicFunction{309448423.855974, 550.279519, 2072.163435}},
          {1486.814377, 1647.616662,
           HyperbolicFunction{5286483.231737, 1245.610950, 2334.107339}}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{{1136.946403, 1491.496431, HyperbolicFunction{318386911.581662, 546.029689, 2065.448511}},
          {1491.496431, 1658.253306,
           HyperbolicFunction{5895782.507577, 1241.361119, 2327.392416}}}}};

    auto begin = find_first_undominated(lhs.functions.begin(), lhs.functions.end(),
                                        rhs.functions.begin(), rhs.functions.end());

    CHECK(std::distance(rhs.functions.begin(), begin) == 1);
}

TEST_CASE("Hyperbolic pieceise functions regression test - Regression 4", "[domination]") {
    PiecewieseDecHypOrLinFunction rhs{
        {{2096.715805, 2388.835866, HyperbolicFunction{2873312399.210743, 748.469368, 3683.699526}},
         {2388.835866, 2443.722122, HyperbolicFunction{2625094759.770883, 797.134446, 3715.378974}},
         {2443.722122, 2708.227518, HyperbolicFunction{2656648298.053953, 790.563397, 3711.515081}},
         {2708.227518, 2939.049455, HyperbolicFunction{296502020.047410, 1784.939768, 4086.114614}},
         {2939.049455, 3331.461339,
          HyperbolicFunction{314685414.702938, 1761.813804, 4081.654086}}}};

    PiecewieseDecHypOrLinFunction lhs_1{
        {{2057.689013, 2479.967146, HyperbolicFunction{3137154613.852615, 790.854617, 3533.215879}},
         {2479.967146, 2537.373066, HyperbolicFunction{3003476454.261966, 815.195440, 3549.060996}},
         {2537.373066, 2813.972854, HyperbolicFunction{3037987511.060497, 808.624391, 3545.197103}},
         {2813.972854, 3051.803729, HyperbolicFunction{324340444.609357, 1862.649357, 3942.267338}},
         {3051.803729, 3455.897174,
          HyperbolicFunction{343633565.800040, 1839.523393, 3937.806810}}}};

    PiecewieseDecHypOrLinFunction lhs_2{
        {{2039.818127, 2414.224004, HyperbolicFunction{2186593841.202531, 916.600495, 3739.004023}},
         {2414.224004, 2461.465639,
          HyperbolicFunction{1673897731.016921, 1044.216596, 3822.078123}},
         {2461.465639, 2689.276854,
          HyperbolicFunction{1697288836.267426, 1037.645547, 3818.214230}}}};

    auto begin_1 = find_first_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                          rhs.functions.begin(), rhs.functions.end());
    auto end_1 = find_last_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                       rhs.functions.begin(), rhs.functions.end());

    auto end_2 =
        find_last_undominated(lhs_2.functions.begin(), lhs_2.functions.end(), begin_1, end_1);

    // two undominated segments from the beginning
    CHECK(std::distance(rhs.functions.begin(), begin_1) == 0);
    CHECK(std::distance(rhs.functions.begin(), end_1) == 2);

    CHECK(std::distance(rhs.functions.begin(), end_2) == 1);
}

TEST_CASE("Hyperbolic piecewise function joint dominance - Regression test 5", "[domination]") {
    PiecewieseDecHypOrLinFunction lhs_1{
        {{1750.282826, 1765.858788, HyperbolicFunction{10939122.356742, 1558.179287, 4890.395846}},
         {1765.858788, 1775.034874, HyperbolicFunction{12507925.405528, 1548.691421, 4878.808870}},
         {1775.034874, 1789.778468, HyperbolicFunction{16245609.737789, 1528.079670, 4856.575958}},
         {1789.778468, 1831.522429,
          HyperbolicFunction{41521345.334197, 1431.973092, 4769.462762}}}};

    PiecewieseDecHypOrLinFunction lhs_2{
        {{1755.844639, 1810.136348, HyperbolicFunction{463253097.936786, 1086.246893, 4174.870804}},
         {1810.136348, 1841.124122, HyperbolicFunction{481708165.964711, 1076.759027, 4163.283827}},
         {1841.124122, 1887.988411, HyperbolicFunction{521737446.652425, 1056.147277, 4141.050915}},
         {1887.988411, 1989.132318, HyperbolicFunction{590622022.836033, 1021.040639, 4109.229457}},
         {1989.132318, 2165.470232,
          HyperbolicFunction{446177108.479564, 1107.442748, 4165.474535}}}};

    PiecewieseDecHypOrLinFunction lhs_3{
        {{1759.095158, 1765.194178, HyperbolicFunction{3672765.111733, 1614.751671, 4981.715145}},
         {1765.194178, 1775.406373, HyperbolicFunction{5398624.653628, 1594.139921, 4959.482233}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{1760.982129, 1768.604452, HyperbolicFunction{7169269.050082, 1580.587156, 4935.027910}},
         {1768.604452, 1781.059918, HyperbolicFunction{9795030.471619, 1559.975405, 4912.794998}},
         {1781.059918, 1818.065545, HyperbolicFunction{28926358.931990, 1463.868827, 4825.681802}},
         {1818.065545, 1871.624467, HyperbolicFunction{12501601.789320, 1550.270936, 4881.926880}},
         {1871.624467, 1929.692995, HyperbolicFunction{248950.307667, 1784.521674, 4970.173362}}}};

    auto begin_1 = find_first_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                          rhs.functions.begin(), rhs.functions.end());
    auto end_1 = find_last_undominated(lhs_1.functions.begin(), lhs_1.functions.end(),
                                       rhs.functions.begin(), rhs.functions.end());

    CHECK(std::distance(rhs.functions.begin(), begin_1) == 0);
    CHECK(std::distance(rhs.functions.begin(), end_1) == 5);

    auto begin_2 = find_first_undominated(lhs_2.functions.begin(), lhs_2.functions.end(),
                                          rhs.functions.begin(), rhs.functions.end());
    auto end_2 = find_last_undominated(lhs_2.functions.begin(), lhs_2.functions.end(),
                                       rhs.functions.begin(), rhs.functions.end());

    CHECK(std::distance(rhs.functions.begin(), begin_2) == 0);
    CHECK(std::distance(rhs.functions.begin(), end_2) == 3);

    auto begin_3 = find_first_undominated(lhs_3.functions.begin(), lhs_3.functions.end(),
                                          rhs.functions.begin(), rhs.functions.end());
    auto end_3 = find_last_undominated(lhs_3.functions.begin(), lhs_3.functions.end(),
                                       rhs.functions.begin(), rhs.functions.end());

    CHECK(std::distance(rhs.functions.begin(), begin_3) == 1);
    CHECK(std::distance(rhs.functions.begin(), end_3) == 5);

    std::vector<PiecewieseDecHypOrLinFunction> lhs_range = {lhs_1, lhs_2, lhs_3};

    auto[begin_4, end_4] =
        find_undominated_range(lhs_range.begin(), lhs_range.end(), rhs.begin(), rhs.end());
    CHECK(std::distance(rhs.begin(), begin_4) == 1);
    CHECK(std::distance(rhs.begin(), end_4) == 1);
}

TEST_CASE("Last dominated segment - Regression test 6", "[domination]") {
    PiecewieseDecHypOrLinFunction lhs{
        {{492.556983, 493.329116, HyperbolicFunction{377491.802097, 430.786356, 727.653156}},
         {493.329116, 506.440496, HyperbolicFunction{1707532.834218, 389.894900, 664.556121}},
         {506.440496, 529.151086, HyperbolicFunction{2050418.192844, 382.564547, 656.649219}},
         {529.151086, 540.055474, HyperbolicFunction{105505.253339, 474.629146, 716.580400}},
         {540.055474, 543.441993, HyperbolicFunction{202.260322, 529.895917, 739.268108}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{491.604834, 492.180728, HyperbolicFunction{156624.277192, 445.533315, 893.276883}},
         {492.180728, 509.153121, HyperbolicFunction{3703848.067113, 358.287403, 758.653199}},
         {509.153121, 555.468476, HyperbolicFunction{9517226.170431, 302.515382, 698.494582}},
         {555.468476, 600.313683, HyperbolicFunction{12947279.875356, 275.185937, 682.424384}},
         {600.313683, 611.968938, HyperbolicFunction{8245.472265, 565.347917, 798.161692}}}};

    auto begin = rhs.begin() + 3;
    auto end = find_last_undominated(lhs.begin(), lhs.end(), begin, rhs.end());

    CHECK(std::distance(rhs.begin(), begin) == 3);
    CHECK(std::distance(rhs.begin(), end) == 3);
}

TEST_CASE("Set domination creates bigger label - Regression test 7", "[domination]") {
    PiecewieseDecHypOrLinFunction lhs_1{
        {{1268.043493, 1296.339739, HyperbolicFunction{3115086.114939, 1180.123016, 3007.635337}},
         {1296.339739, 1305.942513, HyperbolicFunction{4323700.824463, 1166.702287, 2981.001100}},
         {1305.942513, 1308.422868, HyperbolicFunction{12513335.651206, 1107.514094, 2886.203670}},
         {1308.422868, 1383.169467, HyperbolicFunction{316372125.975902, 718.755254, 2286.334334}},
         {1383.169467, 1515.203610, HyperbolicFunction{402917301.215355, 662.983233, 2226.175717}},
         {1515.203610, 1597.241061, HyperbolicFunction{44926921.656182, 1105.016355, 2513.925488}},
         {1597.241061, 1670.594523, HyperbolicFunction{501824.719102, 1487.210869, 2657.905062}}}};
    PiecewieseDecHypOrLinFunction lhs_2{
        {{1258.741678, 1287.037924, HyperbolicFunction{3115086.114939, 1170.821201, 3091.670662}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{1252.810869, 1281.107115, HyperbolicFunction{3115086.114939, 1164.890392, 3135.155279}}}};

    std::vector<PiecewieseDecHypOrLinFunction> lhs_range = {lhs_1, lhs_2};

    auto[begin, end] =
        find_undominated_range(lhs_range.begin(), lhs_range.end(), rhs.begin(), rhs.end());
    CHECK(std::distance(rhs.begin(), begin) == 0);
    CHECK(std::distance(rhs.begin(), end) == 1);
}

TEST_CASE("Find last dominates undominated part - Regression test 8", "[domination]") {
    PiecewieseDecHypOrLinFunction lhs_1{
        {{1478.641192, 1603.368921, HyperbolicFunction{409882879.643027, 835.813666, 2746.367345}},
         {1603.368921, 1874.042983, HyperbolicFunction{457334537.621586, 807.268739, 2720.493572}},
         {1874.042983, 2015.024638, HyperbolicFunction{3562689.977540, 1662.570499, 3042.701188}}}};

    PiecewieseDecHypOrLinFunction rhs{
        {{1531.317504, 1596.989364, HyperbolicFunction{59828577.186906, 1192.854844, 3068.089759}},
         {1596.989364, 1668.021826, HyperbolicFunction{86947659.006804, 1139.224606, 3019.478088}},
         {1668.021826, 1753.969191,
          HyperbolicFunction{91144015.283192, 1130.850794, 3014.554138}}}};

    auto begin_1 = find_first_undominated(lhs_1.begin(), lhs_1.end(), rhs.begin(), rhs.end());
    auto end_1 = find_last_undominated(lhs_1.begin(), lhs_1.end(), rhs.begin(), rhs.end());

    CHECK(std::distance(rhs.begin(), begin_1) == 0);
    CHECK(std::distance(rhs.begin(), end_1) == 3);
}

TEST_CASE("Piecewiese domination test between two hyp with critical point", "[domination]") {
    // these function intersect each other twice between 1.25 and 3
    PiecewieseDecHypOrLinFunction lhs {{{ 1, 4, HyperbolicFunction {640, 0.5, 0}}}};
    PiecewieseDecHypOrLinFunction rhs {{{ 1.25, 3, HyperbolicFunction {64, 1, 200}}}};

    auto begin = find_first_undominated(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    auto end = find_last_undominated(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    CHECK(std::distance(rhs.begin(), begin) == 0);
    CHECK(std::distance(rhs.begin(), end) == 1);
}
