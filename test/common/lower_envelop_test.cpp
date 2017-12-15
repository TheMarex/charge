#include "common/lower_envelop.hpp"

#include "../helper/function_comparator.hpp"
#include "../helper/function_printer.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Lower envelop of tuples", "[lower envelop]") {

    //   4      4         8          1
    //   |
    //   3          0
    //   |  5
    //   2
    //   |                   6
    //   1    2        3
    //   |                        7
    //   0___1___2___3___4___5___6___7

    std::vector<std::tuple<std::uint32_t, std::uint32_t>> values = {
        {3, 3},      {7, 4},   {1.33, 1},   {3.66, 1}, {1.66, 4},
        {0.66, 2.5}, {5, 1.5}, {6.33, 0.5}, {4.3, 4.0}};

    std::vector<std::tuple<std::uint32_t, std::uint32_t>> reference = {values[5], values[2],
                                                                       values[7]};

    auto result = lower_envelop(values);

    REQUIRE(result == reference);
}

TEST_CASE("Lower envelop of functions", "[lower envelop]") {

    //   4  .
    //   |  .  \
    //   3  . \ \...................... 4
    //   |  .  \ `.
    //   2   .  \  `................... 1
    //   |    \`.\..................... 3
    //   1     \.`..................... 2, 5
    //   |         \
    //   0___1___2__\3___4___5___6___7. 0

    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> values = {
        {2.3, 3, LinearFunction {-1.5, 4.5}},
        {2, 3, LinearFunction {-1, 5}},
        {0.5, 2, HyperbolicFunction {4, 0, 0}},
        {1.5, 2.3, LinearFunction {-2.5, 7.25}},
        {1.3,2, LinearFunction{-1/0.7, 5.85}},
        {1, 1.6, LinearFunction {-1/0.6, 3 + 2/3.}},
        {5, 10, HyperbolicFunction {16, 4, -1}},
    };

    auto inf = std::numeric_limits<double>::infinity();

    REQUIRE(values[0](1.7) == inf);
    REQUIRE(values[1](1.7) == inf);
    REQUIRE(values[2](1.7) == Approx(1.38408));
    REQUIRE(values[3](1.7) == Approx(3.0));
    REQUIRE(values[4](1.7) == Approx(3.42143));
    REQUIRE(values[5](1.7) == Approx(1.0));

    REQUIRE(values[0](2.3) == Approx(1.05));
    REQUIRE(values[1](2.3) == Approx(2.7));
    REQUIRE(values[2](2.3) == Approx(1.0));
    REQUIRE(values[3](2.3) == Approx(1.5));
    REQUIRE(values[4](2.3) == Approx(2.99286));
    REQUIRE(values[5](2.3) == Approx(1.0));

    REQUIRE(values[6](8.1) == Approx(-0.0481858));

    Samples samples {values, [](auto lhs, auto rhs) {return std::min<double>(lhs, rhs); }, 0, 10};

    auto result = lower_envelop(values);

    REQUIRE(samples == result);
}

TEST_CASE("Lower envelop of hyperbolic and linear function", "[lower envelop]") {
    //6 -
    //  |   x
    //5 -   `
    //  |`  `
    //4 - `  \
    //  |   ` \
    //3 -     `\
    //  |       x
    //2 -         ` .
    //  |            `` .-----------------------
    //1 -                 `
    //  |                       `  .
    //0 +---|---|---|---|---|---|---|---|---|---|--->
    //      1   2   3   4   5   6   7   8   9   10
    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> values = {
        {1, 4, HyperbolicFunction {4, 0, 1.5}},
        {0, 2, LinearFunction{-1, 4.5}},
        {2, 7, LinearFunction{-0.5, 3.5}}
    };

    // intersection at x=2
    REQUIRE(values[0](2) == values[1](2));
    REQUIRE(values[1](2) == values[2](2));
    // intersection at x=3.23
    REQUIRE(Approx(values[0](3.236)) == values[2](3.236));

    auto result = lower_envelop(values);

    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> reference = {
        {{0, 2, LinearFunction{-1, 0, 4.5}},
         {2, 3.2360679775, HyperbolicFunction{4, 0, 1.5}},
         {3.2360679775, 7, LinearFunction{-0.5, 0, 3.5}}}};

    REQUIRE(ApproxFunction(result) == reference);
}

TEST_CASE("Lower envelop of hyperbolic and linear function - intersection on clamped", "[lower envelop]") {
    //6 -
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
    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> values = {
        {1, 4, HyperbolicFunction {4, 0, 1.5}},
        {3, 7, LinearFunction{-2, 13}},
    };

    // intersection at x=5.625
    REQUIRE(values[0](5.625) == values[1](5.625));

    auto result = lower_envelop(values);

    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> reference = {{
        {1, 4, HyperbolicFunction {4, 0, 1.5}},
        {5.625, 7, LinearFunction{-2, 13}}
    }};

    REQUIRE(result.functions == reference.functions);
}

TEST_CASE("Lower envelop of two linear functions", "[lower envelop]") {
    //6 -
    //  |
    //5 -
    //  |  `
    //4 -    `
    //  |      `
    //3 -        `  \
    //  |          ` \
    //2 -            `\
    //  |              \
    //1 -               \`
    //  |                \ `
    //0 +---|---|---|---|---|---|---|---|---|---|--->
    //      1   2   3   4   5   6   7   8   9   10
    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> values = {
        {0, 5, LinearFunction {-1, 5}},
        {3, 5, LinearFunction{-2, 9}},
    };

    // intersection at x=4
    REQUIRE(values[0](4) == values[1](4));

    auto result = lower_envelop(values);

    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> reference = {{
        {0, 4, values[0].function},
        {4, 5, values[1].function}
    }};

    REQUIRE(result.functions == reference.functions);
}

TEST_CASE("Lower envelop of two hyperbolic functions", "[lower envelop]") {
    //6 - `
    //  |  `
    //5 -  `
    //  |  `
    //4 -   `
    //  |   `
    //3 -   `
    //  |   ``
    //2 -     `
    //  |      ``
    //1 -       `   `   .   .   .   .   .
    //  |           `   .
    //0 +---|---|---|---|--`|---|---|---|---|---|--->
    //      1   2   3   4   5   6   7   8   9   10
    std::vector<LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound>> values = {
        {0.25, 3, HyperbolicFunction {4, 0, 0}},
        {0.3, 5, HyperbolicFunction {1, 0.25, 1}},
    };

    // intersection at x=0.51834f x=1.61223f
    CHECK(values[0](0.51834f) == Approx(values[1](0.51834f)));
    CHECK(values[0](1.61223f) == Approx(values[1](1.61223f)));

    auto result = lower_envelop(values);

    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> reference = {{
        {0.25, 0.518337011337f, values[0].function},
        {0.518337011337f, 1.61222612858f, values[1].function},
        {1.61222612858f, 3, values[0].function},
    }};

    REQUIRE(ApproxFunction(result) == reference);
}
