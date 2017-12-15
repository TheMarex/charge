#include "common/hyp_lin_function.hpp"
#include "common/hyperbolic_function.hpp"
#include "common/interpolating_function.hpp"
#include "common/limited_function.hpp"
#include "common/linear_function.hpp"
#include "common/minimize_combined_function.hpp"
#include "common/piecewise_function.hpp"

#include "ev/limited_tradeoff_function.hpp"

#include "../helper/function_comparator.hpp"
#include "../helper/function_printer.hpp"

#include "catch.hpp"

#include <vector>

using namespace charge;
using namespace charge::common;

TEST_CASE("Use as linear function", "[HypLinFunction]") {
    LinearFunction lin{-1, 5};
    HypOrLinFunction lin_or_hyp{lin};

    REQUIRE(lin(0) == Approx(5.0f));
    REQUIRE(lin(5) == Approx(0.0f));
    REQUIRE(lin_or_hyp(0) == Approx(5.0f));
    REQUIRE(lin_or_hyp(5) == Approx(0.0f));
}

TEST_CASE("Use as hyperbolic function", "[HypLinFunction]") {
    HyperbolicFunction hyp{1, 5, -1};
    HypOrLinFunction lin_or_hyp{hyp};

    REQUIRE(hyp(6) == Approx(0));
    REQUIRE(hyp(7) == Approx(1 / 4.0f - 1.0f));
    REQUIRE(lin_or_hyp(6) == Approx(0));
    REQUIRE(lin_or_hyp(7) == Approx(1 / 4.0f - 1.0f));
}

TEST_CASE("Limited function wrapper can convert HypOrLinFunction", "[HypLinFunction]") {
    const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> limited_hyplin_1{
        5, 10, HyperbolicFunction{1, 5, -1}};
    REQUIRE(limited_hyplin_1(6) == Approx(0.));
    REQUIRE(limited_hyplin_1(7) == Approx(1 / 4.0f - 1.0f));

    auto limited_hyp =
        static_cast<LimitedFunction<HyperbolicFunction, inf_bound, clamp_bound>>(limited_hyplin_1);
    REQUIRE(limited_hyp(6) == Approx(0.));
    REQUIRE(limited_hyp(7) == Approx(1 / 4.0f - 1.0f));

    const LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> limited_hyplin_2{
        0, 5, LinearFunction{-1, 5}};
    REQUIRE(limited_hyplin_2(0) == Approx(5.0f));
    REQUIRE(limited_hyplin_2(5) == Approx(0.0f));

    auto limited_lin =
        static_cast<LimitedFunction<LinearFunction, inf_bound, clamp_bound>>(limited_hyplin_2);
    REQUIRE(limited_lin(0) == Approx(5.0f));
    REQUIRE(limited_lin(5) == Approx(0.0f));
}

TEST_CASE("Combine two limited function", "[HypLinFunction]") {
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> lhs{6, 10,
                                                                  HyperbolicFunction{1, 5, -1}};
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> rhs{0, 5, LinearFunction{-1, 5}};

    auto[first, second, third] = combine_minimal(static_cast<LimitedHyperbolicFunction>(lhs),
                                                 static_cast<LimitedLinearFunction>(rhs));

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);
}

TEST_CASE("Infinity on limited functions", "[LimitedFunction]") {

    constexpr auto inf = std::numeric_limits<double>::infinity();
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> lhs{6, inf, LinearFunction{0, 5, 18}};
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> rhs{9, inf, LinearFunction{0, 8, 27}};

    REQUIRE(lhs(inf) == 18);
    REQUIRE(rhs(inf) == 27);
    REQUIRE(lhs(inf) <= rhs(inf));
}

TEST_CASE("Inverse functions", "[HypLinFunction]") {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    HypOrLinFunction f{HyperbolicFunction{8, 1, -1}};
    HypOrLinFunction g{LinearFunction{-1, 1, 20}};

    auto inv_f = f.inverse();
    auto inv_g = g.inverse();

    REQUIRE(f(inv_f(3)) == Approx(3));
    REQUIRE(inv_f(f(3)) == Approx(3));
    REQUIRE(f(inv_f(9)) == Approx(9));
    REQUIRE(inv_f(f(9)) == Approx(9));

    REQUIRE(g(inv_g(9)) == Approx(9));
    REQUIRE(inv_g(g(9)) == Approx(9));
    REQUIRE(g(inv_g(13)) == Approx(13));
    REQUIRE(inv_g(g(13)) == Approx(13));
}

TEST_CASE("Limit from y values on limited functions", "[LimitedFunction]") {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> hyp{3, 9,
                                                                  HyperbolicFunction{8, 1, -1}};
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> lin{9, 13, LinearFunction{-1, 1, 20}};
    LimitedFunction<HypOrLinFunction, inf_bound, clamp_bound> con{9, inf, ConstantFunction{3}};

    REQUIRE(lin(9) == 12);
    REQUIRE(lin(13) == 8);
    REQUIRE(hyp(3) == 1);
    REQUIRE(hyp(9) == Approx(-7 / 8.0f));
    REQUIRE(con(9) == 3);
    REQUIRE(con(20) == 3);

    // x = b + sqrt(a / (y-c))
    REQUIRE(hyp(1 + sqrt(8 / (0.5 + 1))) == Approx(0.5));
    REQUIRE(hyp(1 + sqrt(8)) == Approx(0));
    hyp.limit_from_y(0, 0.5);
    REQUIRE(hyp.min_x == Approx(1 + sqrt(8 / (0.5 + 1))));
    REQUIRE(hyp.max_x == Approx(1 + sqrt(8)));

    // y = d(x-b) + c
    // (y - c) / d + b = x
    REQUIRE(lin(10) == 11);
    REQUIRE(lin(12) == 9);
    lin.limit_from_y(9, 11);
    REQUIRE(lin.min_x == 10);
    REQUIRE(lin.max_x == 12);

    con.limit_from_y(2, 10);
    REQUIRE(con.min_x == 9);
    REQUIRE(con.max_x == inf);
}

TEST_CASE("Linear piecewise function", "[PiecewieseFunction]") {
    PiecewieseFunction<LinearFunction, inf_bound, clamp_bound> f{
        {{1, 2, LinearFunction{-1, 5}},
         {2, 4, LinearFunction{-0.5, 4}},
         {4, 10, LinearFunction{-0.25, 3}}}};

    constexpr auto inf = std::numeric_limits<double>::infinity();
    REQUIRE(f(0) == inf);

    REQUIRE(f(1) == 4);
    REQUIRE(f(1.5) == 3.5);
    REQUIRE(f(2) == 3);

    REQUIRE(f(2) == 3);
    REQUIRE(f(3) == 2.5);
    REQUIRE(f(4) == 2);

    REQUIRE(f(4) == 2);
    REQUIRE(f(7) == 1.25);
    REQUIRE(f(10) == 0.5);

    auto clipped_f = f.clip(3);
    REQUIRE(clipped_f(3) == 2.5);
    REQUIRE(clipped_f(4) == 2);
    REQUIRE(clipped_f(4) == 2);
    REQUIRE(clipped_f(7) == 1.25);
    REQUIRE(clipped_f(10) == 0.5);

    clipped_f.shift(2);
    REQUIRE(clipped_f(5) == 2.5);
    REQUIRE(clipped_f(6) == 2);
    REQUIRE(clipped_f(6) == 2);
    REQUIRE(clipped_f(9) == 1.25);
    REQUIRE(clipped_f(12) == 0.5);
}

TEST_CASE("Limit from y values on piecewise linear functions", "[PiecewiseFunction]") {
    // 8 -                                                        <---
    //   |                                                           5
    // 7 -                                                  <---  <---
    //   |                                                      |
    // 6 x                                                      |
    //   |`   <------------------------------          <---     |
    // 5 -  `                                |            |     |
    //   |    `                              |            |     3
    // 4 -      `x                           |            |     |
    //   |         `                         1            2     |
    // 3 -             `                     |            |     |
    //   |                 `                 |            |     |
    // 2 -                      `x           |            |     |
    //   |                            ` <----             | <---
    // 1 -                                    `  x        |      <---
    //   |                                                |          4
    // 0 +---|---|---|---|---|---|---|---|---|---|---> <---      <---
    //       1   2   3   4   5   6   7   8   9   10

    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> f{
        {{0, 2, LinearFunction{-1, 0, 6}},
         {2, 6, LinearFunction{-0.5, 2, 4}},
         {6, 10, LinearFunction{-0.25, 6, 2}}}};

    CHECK(f(1) > f(2));
    CHECK(f(2) > f(6));
    CHECK(f(6) > f(10));

    auto clipped_f_1 = f;
    clipped_f_1.limit_from_y(1.5, 5.5);
    CHECK(clipped_f_1(clipped_f_1.min_x()) == 5.5);
    CHECK(clipped_f_1(clipped_f_1.max_x()) == 1.5);

    auto clipped_f_2 = f;
    clipped_f_2.limit_from_y(0, 5.5);
    CHECK(clipped_f_2(clipped_f_2.min_x()) == 5.5);
    CHECK(clipped_f_2(clipped_f_2.max_x()) == 1);

    auto clipped_f_3 = f;
    clipped_f_3.limit_from_y(1.5, 7);
    CHECK(clipped_f_3(clipped_f_3.min_x()) == 6);
    CHECK(clipped_f_3(clipped_f_3.max_x()) == 1.5);

    auto clipped_f_4 = f;
    clipped_f_4.limit_from_y(0, 1);
    CHECK(clipped_f_4.functions.empty());

    auto clipped_f_5 = f;
    clipped_f_5.limit_from_y(7, 8);
    CHECK(clipped_f_5.size() == 1);
    CHECK(clipped_f_5.min_x() == 0);
    CHECK(clipped_f_5(clipped_f_5.min_x()) == 7);
}

TEST_CASE("Limit from y values on piecewise constant functions - Regression 1",
          "[PiecewiseFunction]") {
    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> f{{
        {1, 1, LinearFunction{0, 0, 500}},
    }};

    auto f_clipped = f;
    f_clipped.limit_from_y(0, 2000);

    CHECK(f.functions == f_clipped.functions);
}

TEST_CASE("Limit form y values on piecewise linear function with b < 0 - Regression 2",
          "[LimitedFunction]") {
    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> f{
        {{3.0, 3.25, LinearFunction{-400.000000, -0.750000, 3900.000000}},
         {3.25, 11.25, LinearFunction{-50.000000, 3.250000, 2300.000000}}}};

    f.limit_from_y(0, 2000);

    REQUIRE(f.min_x() == 9.25);
    REQUIRE(f.max_x() == 11.25);
}

TEST_CASE("Piecewise inverse", "[PiecewieseFunction]") {
    PiecewieseFunction<HypOrLinFunction, inf_bound, clamp_bound, monotone_decreasing> f{
        {{0, 2, LinearFunction{-1, 0, 6}},
         {2, 6, LinearFunction{-0.5, 2, 4}},
         {6, 10, LinearFunction{-0.25, 6, 2}}}};

    CHECK(f.inverse(7) == -std::numeric_limits<double>::infinity());
    CHECK(f.inverse(5) == Approx(1));
    CHECK(f.inverse(2) == Approx(6));
    CHECK(f.inverse(1) == Approx(10));
    CHECK(f.inverse(0) == std::numeric_limits<double>::infinity());
}

TEST_CASE("Interpolating function push_back", "[Interpolating function]") {
    PiecewieseFunction<LinearFunction, inf_bound, clamp_bound, monotone_decreasing> f{
        {{0, 2, LinearFunction{-1, 0, 6}},
         {2, 6, LinearFunction{-0.5, 2, 4}},
         {6, 10, LinearFunction{-0.25, 6, 2}}}};

    InterpolatingFunction<inf_bound, clamp_bound, monotone_decreasing> f_interpolating;
    for (const auto &sub_f : f.functions) {
        f_interpolating.push_back(sub_f);
    }

    CHECK(f_interpolating(0) == f(0));
    CHECK(f_interpolating(0.1) == f(0.1));
    CHECK(f(10) == 1);
    CHECK(f(6) == 2);

    Samples f_samples(f, -1, 11, 0.01);
    CHECK(f_samples == f_interpolating);
}

TEST_CASE("Interpolating function limit x", "[Interpolating function]") {
    InterpolatingFunction<inf_bound, clamp_bound, monotone_decreasing> f{
        {std::tuple<double, double>{0, 6}, {2, 4}, {6, 2}, {10, 1}}};

    f.limit_from_x(-1, 11);
    CHECK(f.min_x() == 0);
    CHECK(f.max_x() == 10);
    CHECK(f(f.min_x()) == 6);
    CHECK(f(f.max_x()) == 1);

    f.limit_from_x(3, 11);
    CHECK(f.min_x() == 3);
    CHECK(f.max_x() == 10);
    CHECK(f(f.min_x()) == 3.5);
    CHECK(f(f.max_x()) == 1);

    f.limit_from_x(4, 9);
    CHECK(f.min_x() == 4);
    CHECK(f.max_x() == 9);
    CHECK(f(f.min_x()) == 3);
    CHECK(f(f.max_x()) == 1.25);

    f.limit_from_x(4, 4);
    CHECK(f.min_x() == 4);
    CHECK(f.max_x() == 4);
    CHECK(f(f.min_x()) == 3);
    CHECK(f(f.max_x()) == 3);
}

TEST_CASE("Check inverse derivative - Regression 1", "[piecewise function]") {
    PiecewieseDecHypOrLinFunction pwf{{{89.759788482123327, 90.059172970775876,
                                        HyperbolicFunction{15.382411, 87.634159, 177.143094}},
                                       {90.059172970775876, 111.49464575766206,
                                        HyperbolicFunction{143394.600513, 39.022333, 124.707837}},
                                       {111.49464575766206, 121.38338401118065,
                                        HyperbolicFunction{5035.850236, 81.828431, 146.287457}}}};

    CHECK(pwf.begin()[0]->deriv(pwf.begin()[0].min_x) == Approx(-3.203).epsilon(0.001));
    CHECK(pwf.begin()[1]->deriv(pwf.begin()[1].min_x) == Approx(-2.157).epsilon(0.001));
    CHECK(pwf.begin()[2]->deriv(pwf.begin()[2].min_x) == Approx(-0.385).epsilon(0.001));

    CHECK(pwf.inverse_deriv(-4) == -std::numeric_limits<double>::infinity());
    CHECK(pwf.inverse_deriv(-3.0) == Approx(89.80).epsilon(0.01));
    CHECK(pwf.inverse_deriv(-2.0) == Approx(91.36).epsilon(0.01));
    CHECK(pwf.inverse_deriv(-0.3) == Approx(114.08).epsilon(0.01));
}

TEST_CASE("Inverse derivate - Regression 2", "[PiecewieseFunction]") {
    PiecewieseDecHypOrLinFunction pwf{
        {{881.542491, 887.899010, HyperbolicFunction{249071591.217054, 530.862392, 1974.641285}},
         {887.899010, 911.060094, HyperbolicFunction{301482379.334996, 507.395495, 1846.218649}},
         {911.060094, 912.830735, HyperbolicFunction{28.909584, 905.748170, 3695.402312}}}};

    auto slope = -6.666666666666667;
    auto x = pwf.inverse_deriv(slope);

    auto &sub = pwf.sub(x);
    CHECK(pwf.sub(882)->deriv(822) < slope);
    CHECK(pwf.sub(911)->deriv(911) < slope);
    CHECK(sub->deriv(x) >= Approx(slope));
}

TEST_CASE("Inverse derivate - Regression 3", "[PiecewieseFunction]")
{
    PiecewieseDecHypOrLinFunction f {{{873.014523, 911.921588, LinearFunction{-33.333333, 815.921588, 4038.472423}},{911.921588, 917.714153, HyperbolicFunction{1012.191918, 894.543894, 835.120619}}}};

    auto x = f.inverse_deriv(-20);
    CHECK(x == Approx(911.92));
}

TEST_CASE("Clip negative constant function - Regression 1", "[PiecewieseFunction]")
{
    PiecewieseDecHypOrLinFunction f {{{2.5006817909001908, 2.5006817909001908, ConstantFunction{-0.14407100726148525}}}};

    f.limit_from_y(0, 1000);
    CHECK(f.min_x() == Approx(2.50068));
    CHECK(f.max_x() == Approx(2.50068));
    CHECK(f(2.50069) == 0);
}
