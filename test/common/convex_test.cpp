#include "common/convex.hpp"
#include "common/piecewise_functions_aliases.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Convex linear piecewise function", "[convex]") {
    // 8 -
    //   |
    // 7 -
    //   |
    // 6 x
    //   |`
    // 5 -  `
    //   |    `
    // 4 -      `x
    //   |         `
    // 3 -             `
    //   |                 `
    // 2 -                      `x
    //   |                            `
    // 1 -                                    `  x
    //   |
    // 0 +---|---|---|---|---|---|---|---|---|---|--->
    //       1   2   3   4   5   6   7   8   9   10

    PiecewieseDecLinearFunction f{{{0, 2, LinearFunction{-1, 0, 6}},
                                   {2, 6, LinearFunction{-0.5, 2, 4}},
                                   {6, 10, LinearFunction{-0.25, 6, 2}}}};

    CHECK(f(1) > f(2));
    CHECK(f(2) > f(6));
    CHECK(f(6) > f(10));

    CHECK(f.functions[0]->deriv(0) <= f.functions[0]->deriv(2));
    CHECK(f.functions[0]->deriv(2) <= f.functions[1]->deriv(2));
    CHECK(f.functions[1]->deriv(2) <= f.functions[1]->deriv(6));
    CHECK(f.functions[1]->deriv(6) <= f.functions[2]->deriv(6));
    CHECK(f.functions[2]->deriv(6) <= f.functions[2]->deriv(10));

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Convex hyperbolic linear piecewise function", "[convex]") {
    //  8 -
    //    |
    //  4 -   x
    //    |   `
    //  3 -    `
    //    |     `
    //  2 -      `
    //    |       `
    //  1 -        x
    //    |         `
    //  0 -             `
    //    |                 `
    // -1 -                      `x
    //    |                            `
    // -2 -                                    `  x
    //    |
    //  0 +---|---|---|---|---|---|---|---|---|---|--->
    //        1   2   3   4   5   6   7   8   9   10

    PiecewieseDecHypOrLinFunction f{{{1, 2, HyperbolicFunction{4, 0, 0}},
                                     {2, 6, LinearFunction{-0.5, 2, 1}},
                                     {6, 10, LinearFunction{-0.25, 6, -1}}}};

    CHECK(f(1) > f(2));
    CHECK(f(2) > f(6));
    CHECK(f(6) > f(10));

    CHECK(f.functions[0]->deriv(1) <= f.functions[0]->deriv(2));
    CHECK(f.functions[0]->deriv(2) <= f.functions[1]->deriv(2));
    CHECK(f.functions[1]->deriv(2) <= f.functions[1]->deriv(6));
    CHECK(f.functions[1]->deriv(6) <= f.functions[2]->deriv(6));
    CHECK(f.functions[2]->deriv(6) <= f.functions[2]->deriv(10));

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Non-Convex hyperbolic linear piecewise function", "[convex]") {
    //  8 -
    //    |
    //  4 -   x
    //    |   `
    //  3 -   `
    //    |   `
    //  2 -    `
    //    |     ` .
    //  1 -        x
    //    |         `
    //  0 -           `
    //    |            `
    // -1 -             `
    //    |              `
    // -2 -                `
    //    |                 `
    // -3 -                  `
    //    |                   `
    // -4 -                    `
    //    |                     `
    // -5 -                      `x
    //    |                          `
    // -6 -                              `
    //    |                                   `
    // -7 +---|---|---|---|---|---|---|---|---|---x--->
    //        1   2   3   4   5   6   7   8   9   10

    PiecewieseDecHypOrLinFunction f{{{1, 2, HyperbolicFunction{4, 0, 0}},
                                     {2, 6, LinearFunction{-1.5, 2, 1}},
                                     {6, 10, LinearFunction{-0.25, 6, -5}}}};

    CHECK(f(1) > f(2));
    CHECK(f(2) > f(6));
    CHECK(f(6) > f(10));

    CHECK(f.functions[0]->deriv(1) <= f.functions[0]->deriv(2));
    CHECK(f.functions[0]->deriv(2) > f.functions[1]->deriv(2));
    CHECK(f.functions[1]->deriv(2) <= f.functions[1]->deriv(6));
    CHECK(f.functions[1]->deriv(6) <= f.functions[2]->deriv(6));
    CHECK(f.functions[2]->deriv(6) <= f.functions[2]->deriv(10));

    REQUIRE(!is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Mixed piecewise function - Regresion test 1", "[convex]") {
    PiecewieseDecHypOrLinFunction f{
        {{4, 5, HyperbolicFunction{4.000000, 3.000000, 8.500000}},
         {5, 7, LinearFunction{-1.000000, 3.000000, 11.500000}},
         {7, 7.51984215, HyperbolicFunction{4.000000, 5.000000, 6.500000}},
         {7.51984215, 8.51984215, LinearFunction{-0.500000, 6.519842, 7.629961}},
         {8.51984215, 10, HyperbolicFunction{4.000000, 6.000000, 6.000000}}}};

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Hyperbolic functions - Regression test 2", "[convex]") {

    PiecewieseDecHypOrLinFunction f{
        {{716.517761, 721.280090, HyperbolicFunction{798207.875000, 647.464172, 2275.932861}},
         {721.280579, 732.245605, HyperbolicFunction{6437498.000000, 573.249695, 2128.649658}},
         {732.335938, 736.820068, HyperbolicFunction{74009040.000000, 373.488861, 1808.566772}},
         {736.853027, 820.244385, HyperbolicFunction{122662288.000000, 306.843414, 1705.733032}},
         {820.244385, 821.275024, HyperbolicFunction{0.528947, 819.408752, 2170.344971}}}};

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Hyperbolic functions - Regression test 3", "[convex]") {
    PiecewieseDecHypOrLinFunction f{
        {{721.268372, 726.348999, HyperbolicFunction{640178.000000, 652.686035, 2232.757568}},
         {726.348999, 728.659241, HyperbolicFunction{10117618.000000, 541.490356, 2054.663574}},
         {728.659241, 818.128845, HyperbolicFunction{151303136.000000, 267.528900, 1631.930908}},
         {818.128845, 886.693115, HyperbolicFunction{183958672.000000, 230.467499, 1598.337646}},
         {886.693176, 987.912415, HyperbolicFunction{84391528.000000, 380.581848, 1696.057373}},
         {987.912415, 990.542908, HyperbolicFunction{23.062639, 983.974182, 1923.366455}}}};

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}

TEST_CASE("Hyperbolic function - Regression test 4", "[convex]") {
    PiecewieseDecHypOrLinFunction f{
        {{948.952332, 950.125122, HyperbolicFunction{11922.144531, 931.946411, 2863.370850}},
         {950.125122, 959.059509, HyperbolicFunction{3481212.500000, 829.522888, 2660.105225}},
         {959.059509, 962.067749, HyperbolicFunction{22343980.000000, 718.327209, 2482.011230}},
         {962.067749, 1051.537231, HyperbolicFunction{151303136.000000, 500.937408, 2146.570557}},
         {1051.537231, 1120.101440, HyperbolicFunction{183958672.000000, 463.875977, 2112.977295}},
         {1120.101562, 1221.297607, HyperbolicFunction{84391528.000000, 613.990295, 2210.697021}},
         {1221.297607, 1222.909912, HyperbolicFunction{5.072988, 1218.944702, 2438.594727}}}};

    REQUIRE(is_convex(f.functions.begin(), f.functions.end()));
}
