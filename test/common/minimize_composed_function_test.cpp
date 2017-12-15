#include "common/minimize_composed_function.hpp"

#include "../helper/function_comparator.hpp"
#include "../helper/function_printer.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Combine linear and PWL function") {
    LimitedHypOrLinFunction f{1, 2, LinearFunction{-0.5, 5}};

    auto capacity = 5;

    PiecewieseDecLinearFunction pwf{{
        {0, 2, LinearFunction(-1, capacity - 0)},
        {2, 6, LinearFunction(-0.5, capacity - 1)},
        {6, 10, LinearFunction(-0.25, capacity - 2.75)},
    }};

    StatefulPiecewieseDecLinearFunction g{pwf};

    REQUIRE(f(1) == pwf(0.5));

    auto[delta, h] = compose_minimal(f, g);

    REQUIRE(h(1) == f(1));
    REQUIRE(h(1) == pwf(0.5));
    REQUIRE(h(2) == pwf(1.5));
    REQUIRE(h(3) == pwf(2.5));
    REQUIRE(h(4) == pwf(3.5));
    REQUIRE(h(5) == pwf(4.5));
    REQUIRE(h(6) == pwf(5.5));
    REQUIRE(h(7) == pwf(6.5));
    REQUIRE(h(8) == pwf(7.5));
    REQUIRE(h(9) == pwf(8.5));
    REQUIRE(h(10) == pwf(9.5));
    REQUIRE(h(10.5) == pwf(10));
}

TEST_CASE("Combine linear and PWL function, charging does not pay off") {
    LimitedHypOrLinFunction f{5, 7, LinearFunction{-0.51, 5.05}};

    auto capacity = 5;

    PiecewieseDecLinearFunction pwf{{
        {0, 2, LinearFunction(-1, capacity - 0)},
        {2, 6, LinearFunction(-0.5, capacity - 1)},
        {6, 10, LinearFunction(-0.25, capacity - 2.75)},
    }};

    StatefulPiecewieseDecLinearFunction g{pwf};

    // f'(5) = -0.51
    // pwf'(3) = -0.5
    // -> does not pay off to charge
    REQUIRE(f(5) == Approx(pwf(3)));

    auto[delta, solution] = compose_minimal(f, g);
    REQUIRE(solution.functions.size() == 0);
}

TEST_CASE("Combine hyperbolic and PWL function, five candidates") {
    LimitedHypOrLinFunction f{3, 8, HyperbolicFunction{64, 0, -1}};

    const auto &f_hyp = static_cast<const HyperbolicFunction &>(*f);

    const auto deriv_f = [&](double x) { return -2 * f_hyp.a / std::pow(x - f_hyp.b, 3); };

    const auto inv_deriv_f = [&](double alpha) {
        return f_hyp.b + std::cbrt(-2 * f_hyp.a / alpha);
    };

    REQUIRE(deriv_f(3) == Approx(-4.7407407407));
    REQUIRE(deriv_f(4) == -2);
    REQUIRE(deriv_f(5) == -1.024);
    REQUIRE(deriv_f(6) == Approx(-0.5925925926));
    REQUIRE(deriv_f(7) == Approx(-0.3731778426));
    REQUIRE(deriv_f(8) == -0.25);

    REQUIRE(f(3) == Approx(6.111111));
    REQUIRE(f(4) == Approx(3));
    REQUIRE(f(5) == Approx(1.56));
    REQUIRE(f(6) == Approx(0.77778));
    REQUIRE(f(7) == Approx(0.30612244897959173));
    REQUIRE(f(8) == Approx(0));

    auto capacity = 10;

    // Using a linear approximation of f we can construct the worst-case
    // where each sub function needs to be considered as a possible starting
    // point for charging.
    auto slope_0 = (f(3) - capacity) / 3;
    auto slope_1 = f(4) - f(3);
    auto slope_2 = f(5) - f(4);
    auto slope_3 = f(6) - f(5);
    auto slope_4 = f(7) - f(6);
    auto slope_5 = f(8) - f(7);

    REQUIRE(slope_1 > deriv_f(3));
    REQUIRE(slope_1 < deriv_f(4));
    REQUIRE(slope_2 > deriv_f(4));
    REQUIRE(slope_2 < deriv_f(5));
    REQUIRE(slope_3 > deriv_f(5));
    REQUIRE(slope_3 < deriv_f(6));
    REQUIRE(slope_4 > deriv_f(6));
    REQUIRE(slope_4 < deriv_f(7));
    REQUIRE(slope_5 > deriv_f(7));
    REQUIRE(slope_5 < deriv_f(8));

    REQUIRE(inv_deriv_f(slope_1) == Approx(3.45222f));
    REQUIRE(inv_deriv_f(slope_2) == Approx(4.46289f));
    REQUIRE(inv_deriv_f(slope_3) == Approx(5.46966f));
    REQUIRE(inv_deriv_f(slope_4) == Approx(6.47433f));
    REQUIRE(inv_deriv_f(slope_5) == Approx(7.47776f));

    PiecewieseDecLinearFunction pwf{{
        {0, 3, LinearFunction(slope_0, capacity)},
        {3, 4, LinearFunction(slope_1, f(3) - slope_1 * 3)},
        {4, 5, LinearFunction(slope_2, f(4) - slope_2 * 4)},
        {5, 6, LinearFunction(slope_3, f(5) - slope_3 * 5)},
        {6, 7, LinearFunction(slope_4, f(6) - slope_4 * 6)},
        {7, 8, LinearFunction(slope_5, f(7) - slope_5 * 7)},
    }};

    REQUIRE(pwf(3) == Approx(f(3)));
    REQUIRE(pwf(4) == Approx(f(4)));
    REQUIRE(pwf(5) == Approx(f(5)));
    REQUIRE(pwf(6) == Approx(f(6)));
    REQUIRE(pwf(7) == Approx(f(7)));
    REQUIRE(pwf(8) == Approx(f(8)));

    StatefulPiecewieseDecLinearFunction g{pwf};

    auto[delta, solution] = compose_minimal(f, g);

    PiecewieseDecHypOrLinFunction reference{
        {{3.45221749599, 3.89261195827,
          LinearFunction{-3.11111111111, -0.107388041731, 15.4444444444}},
         {3.89261195827, 4.89261195827, LinearFunction{-1.44, -0.107388041731, 8.76}},
         {4.89261195827, 5.89261195827,
          LinearFunction{-0.782222222222, -0.107388041731, 5.47111111111}},
         {5.89261195827, 6.89261195827,
          LinearFunction{-0.471655328798, -0.107388041731, 3.60770975057}},
         {6.89261195827, 7.89261195827,
          LinearFunction{-0.30612244898, -0.107388041731, 2.44897959184}}}};

    CHECK(ApproxFunction(reference) == solution);
}

TEST_CASE("Combine constant and PWL function") {
    LimitedHypOrLinFunction f{1, 1, ConstantFunction{4.5}};

    auto capacity = 5;

    PiecewieseDecLinearFunction pwf{{
        {0, 2, LinearFunction(-1, capacity - 0)},
        {2, 6, LinearFunction(-0.5, capacity - 1)},
        {6, 10, LinearFunction(-0.25, capacity - 2.75)},
    }};

    StatefulPiecewieseDecLinearFunction g{pwf};

    REQUIRE(f(1) == pwf(0.5));

    auto[delta, h] = compose_minimal(f, g);

    REQUIRE(h(1) == f(1));
    REQUIRE(h(1) == pwf(0.5));
    REQUIRE(h(2) == pwf(1.5));
    REQUIRE(h(3) == pwf(2.5));
    REQUIRE(h(4) == pwf(3.5));
    REQUIRE(h(5) == pwf(4.5));
    REQUIRE(h(6) == pwf(5.5));
    REQUIRE(h(7) == pwf(6.5));
    REQUIRE(h(8) == pwf(7.5));
    REQUIRE(h(9) == pwf(8.5));
    REQUIRE(h(10) == pwf(9.5));
    REQUIRE(h(10.5) == pwf(10));
}

// Same test case as for the non-convex output
TEST_CASE("Combine hyperbolic and PWL function with five convex solutions") {
    LimitedHypOrLinFunction f{3, 8, HyperbolicFunction{64, 0, -1}};

    const auto &f_hyp = static_cast<const HyperbolicFunction &>(*f);

    const auto deriv_f = [&](double x) { return -2 * f_hyp.a / std::pow(x - f_hyp.b, 3); };

    const auto inv_deriv_f = [&](double alpha) {
        return f_hyp.b + std::cbrt(-2 * f_hyp.a / alpha);
    };

    auto capacity = 10;

    // Using a linear approximation of f we can construct the worst-case
    // where each sub function needs to be considered as a possible starting
    // point for charging.
    auto slope_0 = (f(3) - capacity) / 3;
    auto slope_1 = f(4) - f(3);
    auto slope_2 = f(5) - f(4);
    auto slope_3 = f(6) - f(5);
    auto slope_4 = f(7) - f(6);
    auto slope_5 = f(8) - f(7);

    PiecewieseDecLinearFunction pwf{{
        {0, 3, LinearFunction(slope_0, capacity)},
        {3, 4, LinearFunction(slope_1, f(3) - slope_1 * 3)},
        {4, 5, LinearFunction(slope_2, f(4) - slope_2 * 4)},
        {5, 6, LinearFunction(slope_3, f(5) - slope_3 * 5)},
        {6, 7, LinearFunction(slope_4, f(6) - slope_4 * 6)},
        {7, 8, LinearFunction(slope_5, f(7) - slope_5 * 7)},
    }};

    StatefulPiecewieseDecLinearFunction g{pwf};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, g, std::back_inserter(solutions));

    PiecewieseSolution reference_1{
        {{3.45221749599, 7.89261195827, LinearFunction{0, 0, 3.45221749599}}},
        {{{3.45221749599, 3.89261195827,
           LinearFunction{-3.11111111111, -0.107388041731, 15.4444444444}},
          {3.89261195827, 4.89261195827, LinearFunction{-1.44, -0.107388041731, 8.76}},
          {4.89261195827, 5.89261195827,
           LinearFunction{-0.782222222222, -0.107388041731, 5.47111111111}},
          {5.89261195827, 6.89261195827,
           LinearFunction{-0.471655328798, -0.107388041731, 3.60770975057}},
          {6.89261195827, 7.89261195827,
           LinearFunction{-0.30612244898, -0.107388041731, 2.44897959184}}}}};
    PiecewieseSolution reference_2{
        {{4.46288633388, 7.91655172304, LinearFunction{0, 0, 4.46288633388}}},
        {{{4.46288633388, 4.91655172304, LinearFunction{-1.44, -0.0834482769561, 8.76}},
          {4.91655172304, 5.91655172304,
           LinearFunction{-0.782222222222, -0.0834482769561, 5.47111111111}},
          {5.91655172304, 6.91655172304,
           LinearFunction{-0.471655328798, -0.0834482769561, 3.60770975057}},
          {6.91655172304, 7.91655172304,
           LinearFunction{-0.30612244898, -0.0834482769561, 2.44897959184}}}}};

    PiecewieseSolution reference_3{
        {{5.46965551376, 7.93175506592, LinearFunction{0, 0, 5.46965551376}}},
        {{{5.46965551376, 5.93175506592,
           LinearFunction{-0.782222151756, -0.068244934082, 5.47111082077}},
          {5.93175506592, 6.93175506592,
           LinearFunction{-0.471655368805, -0.068244934082, 3.60770988464}},
          {6.93175506592, 7.93175506592,
           LinearFunction{-0.306122422218, -0.068244934082, 2.44897937775}}}}};

    PiecewieseSolution reference_4{
        {{6.47433328629, 7.94227027893, LinearFunction{0, 0, 6.47433328629}}},
        {{{6.47433328629, 6.94227027893,
           LinearFunction{-0.471655368805, -0.0577297210693, 3.60770988464}},
          {6.94227027893, 7.94227027893,
           LinearFunction{-0.306122422218, -0.0577297210693, 2.44897937775}}}}};

    PiecewieseSolution reference_5{
        {{7.47776126862, 7.94997549057, LinearFunction{0, 0, 7.47776126862}}},
        {{{7.47776126862, 7.94997549057,
           LinearFunction{-0.306122422218, -0.0500245094299, 2.44897937775}}}}};

    CHECK(solutions.size() == 5);
    CHECK(ApproxFunction(std::get<1>(solutions[0])) == std::get<1>(reference_1));
    CHECK(ApproxFunction(std::get<1>(solutions[1])) == std::get<1>(reference_2));
    CHECK(ApproxFunction(std::get<1>(solutions[2])) == std::get<1>(reference_3));
    CHECK(ApproxFunction(std::get<1>(solutions[3])) == std::get<1>(reference_4));
    CHECK(ApproxFunction(std::get<1>(solutions[4])) == std::get<1>(reference_5));

    CHECK(ApproxFunction(std::get<0>(solutions[0])) == std::get<0>(reference_1));
    CHECK(ApproxFunction(std::get<0>(solutions[1])) == std::get<0>(reference_2));
    CHECK(ApproxFunction(std::get<0>(solutions[2])) == std::get<0>(reference_3));
    CHECK(ApproxFunction(std::get<0>(solutions[3])) == std::get<0>(reference_4));
    CHECK(ApproxFunction(std::get<0>(solutions[4])) == std::get<0>(reference_5));

    auto & [ delta_1, h_1 ] = solutions[0];
    CHECK(f(delta_1(h_1.min_x())) == Approx(h_1(h_1.min_x())));
    CHECK(0 == Approx(h_1(h_1.max_x())));
    auto & [ delta_2, h_2 ] = solutions[1];
    CHECK(f(delta_2(h_2.min_x())) == Approx(h_2(h_2.min_x())));
    CHECK(0 == Approx(h_2(h_2.max_x())));
    auto & [ delta_3, h_3 ] = solutions[2];
    CHECK(f(delta_3(h_3.min_x())) == Approx(h_3(h_3.min_x())));
    CHECK(0 == Approx(h_3(h_3.max_x())));
    auto & [ delta_4, h_4 ] = solutions[3];
    CHECK(f(delta_4(h_4.min_x())) == Approx(h_4(h_4.min_x())));
    CHECK(0 == Approx(h_4(h_4.max_x())));
    auto & [ delta_5, h_5 ] = solutions[4];
    CHECK(f(delta_5(h_5.min_x())) == Approx(h_5(h_5.min_x())));
    CHECK(0 == Approx(h_5(h_5.max_x())));
}

TEST_CASE("Regression test of linking to linear function with better slope", "[minimal compose]") {
    PiecewieseDecHypOrLinFunction lhs{
        {{1479.6498021832708, 1508.0545022488018, LinearFunction{-33.333333, 51875.366170315174}},
         {1508.0545022488018, 1509.6078191325887,
          HyperbolicFunction{59502.382604, 1463.008313, 1577.559577}},
         {1509.6078191325887, 1518.3274208221796,
          HyperbolicFunction{95174.45446, 1455.110309, 1572.915411}},
         {1518.3274208221796, 1519.778984471895,
          HyperbolicFunction{73.74026, 1512.521166, 1594.543118}},
         {1519.778984471895, 1523.9688983996118,
          HyperbolicFunction{383.058819, 1507.209243, 1593.518556}}}};

    PiecewieseDecLinearFunction pwf{
        {{0.000000, 527.811766, LinearFunction{-6.062767, 0.000000, 4000.000000}},
         {527.811766, 565.552943, LinearFunction{-5.299252, 527.811766, 800.000000}},
         {565.552943, 617.035296, LinearFunction{-3.884826, 565.552943, 600.000000}},
         {617.035296, 692.800001, LinearFunction{-2.639752, 617.035296, 400.000000}},
         {692.800001, 917.270587, LinearFunction{-0.890985, 692.800001, 200.000000}}}};
    StatefulPiecewieseDecLinearFunction cf{pwf};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(lhs, cf, std::back_inserter(solutions));

    CHECK(solutions.size() == 1);

    auto & [ delta_1, h_1 ] = solutions[0];

    CHECK(h_1.min_x() == Approx(1508.0545022488018));
    CHECK(h_1(h_1.min_x()) == Approx(lhs(1508.0545022488018)));
}

TEST_CASE("Regression test from FPC dijkstra test - 1", "[minimal compose]") {
    PiecewieseDecHypOrLinFunction f{
        {{1.450953, 10.000000, HyperbolicFunction{4000.000000, 0.000000, 100.000000}}}};
    PiecewieseDecLinearFunction g{
        {{0.000000, 4.000000, LinearFunction{-400.000000, 0.000000, 2000.000000}},
         {4.000000, 12.000000, LinearFunction{-50.000000, 4.000000, 400.000000}}}};
    StatefulPiecewieseDecLinearFunction stateful_g{g};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, stateful_g, std::back_inserter(solutions));

    CHECK(solutions.size() == 2);

    auto & [ delta_1, h_1 ] = solutions[0];
    auto & [ delta_2, h_2 ] = solutions[1];

    CHECK(h_1.min_x() == Approx(2.7144176166));
    CHECK(h_1(h_1.min_x()) == Approx(f(h_1.min_x())));

    CHECK(h_2.min_x() == Approx(5.4288352332));
    CHECK(h_2(h_2.min_x()) == Approx(f(h_2.min_x())));
    CHECK(h_2(h_2.max_x()) == Approx(0.0));
}

TEST_CASE("Regression test from FPC dijkstra test - 2", "[minimal compose]") {
    PiecewieseDecHypOrLinFunction f{
        {{1.000000, 1.000000, LinearFunction{0.000000, 0.000000, 500.000000}}}};
    PiecewieseDecLinearFunction g{
        {{0.000000, 4.000000, LinearFunction{-400.000000, 0.000000, 2000.000000}},
         {4.000000, 12.000000, LinearFunction{-50.000000, 4.000000, 400.000000}}}};
    StatefulPiecewieseDecLinearFunction stateful_g{g};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, stateful_g, std::back_inserter(solutions));

    CHECK(solutions.size() == 1);

    auto & [ delta_1, h_1 ] = solutions[0];

    CHECK(h_1.min_x() == Approx(1));
    CHECK(h_1(h_1.min_x()) == Approx(f(h_1.min_x())));
    CHECK(h_1(h_1.max_x()) == Approx(0.0));
}

TEST_CASE("Regression test from FPC dijkstra test - 3", "[minimal compose]") {
    PiecewieseDecHypOrLinFunction f{
        {{1.000000, 1.000000, LinearFunction{0.000000, 0.000000, 100.000000}}}};
    PiecewieseDecLinearFunction g{
        {{0.000000, 4.000000, LinearFunction{-400.000000, 0.000000, 2000.000000}},
         {4.000000, 12.000000, LinearFunction{-50.000000, 4.000000, 400.000000}}}};
    StatefulPiecewieseDecLinearFunction stateful_g{g};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, stateful_g, std::back_inserter(solutions));

    CHECK(solutions.size() == 1);

    auto & [ delta_1, h_1 ] = solutions[0];

    CHECK(h_1.min_x() == Approx(1));
    CHECK(h_1(h_1.min_x()) == Approx(f(h_1.min_x())));
    CHECK(h_1(h_1.max_x()) == Approx(0.0));
}

TEST_CASE("Composition ciritical point is not global minimum") {
    PiecewieseDecHypOrLinFunction lhs{
        {{500, 700, LinearFunction{-6.333333, 0, 7075.36617031517}},
         {700, 1450, LinearFunction{-3.333333, 0, 4975.366170315174}}}};

    PiecewieseDecLinearFunction pwf{
        {{0.000000, 527.811766, LinearFunction{-6.062767, 0.000000, 4000.000000}},
         {527.811766, 565.552943, LinearFunction{-5.299252, 527.811766, 800.000000}},
         {565.552943, 617.035296, LinearFunction{-3.884826, 565.552943, 600.000000}},
         {617.035296, 692.800001, LinearFunction{-2.639752, 617.035296, 400.000000}},
         {692.800001, 917.270587, LinearFunction{-0.890985, 692.800001, 200.000000}}}};
    StatefulPiecewieseDecLinearFunction cf{pwf};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(lhs, cf, std::back_inserter(solutions));

    CHECK(solutions.size() == 2);

    auto & [ delta_1, h_1 ] = solutions[0];
    auto & [ delta_2, h_2 ] = solutions[1];

    CHECK(h_1.min_x() == Approx(700));
    CHECK(h_1(h_1.min_x()) == Approx(lhs(h_1.min_x())));
    CHECK(h_1(h_1.max_x()) == Approx(0.0).epsilon(0.001));
    CHECK(h_2.min_x() == Approx(1450));
    CHECK(h_2(h_2.min_x()) == Approx(lhs(h_2.min_x())));
    CHECK(h_2(h_2.max_x()) == Approx(0.0).epsilon(0.001));
}

TEST_CASE("Luxev node 3840 - Regression 1", "[compose_minimal]") {
    PiecewieseDecHypOrLinFunction f{
        {{831.943906, 864.307012, HyperbolicFunction{25678788.073262, 576.634952, 1352.872133}},
         {864.307012, 879.998096, HyperbolicFunction{48529213.479193, 508.642451, 1279.531867}},
         {879.998096, 931.851598, HyperbolicFunction{55737980.421740, 491.096831, 1262.905327}},
         {931.851598, 1007.950644, HyperbolicFunction{35859796.038277, 551.356371, 1302.132407}},
         {1007.950644, 1060.210259, HyperbolicFunction{3441082.167333, 798.912181, 1395.391151}},
         {1060.210259, 1157.167342, HyperbolicFunction{4746667.229624, 769.339010, 1389.687088}}}};

    PiecewieseDecLinearFunction g{
        {{0.000000, 96.000000, LinearFunction{-33.333333, 0.000000, 4000.000000}}}};
    StatefulPiecewieseDecLinearFunction cf{g};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, cf, std::back_inserter(solutions));

    CHECK(solutions.size() == 1);

    auto & [ delta_1, h_1 ] = solutions[0];
    CHECK(h_1.min_x() == Approx(831.9439));
    CHECK(h_1(h_1.min_x()) == Approx(f(h_1.min_x())));
    CHECK(h_1(h_1.max_x()) == Approx(800).epsilon(0.001));
}

TEST_CASE("Luxev node 3840 - Regression 2", "[compose_minimal]") {
    PiecewieseDecHypOrLinFunction f{
        {{959.982878, 1060.411220, HyperbolicFunction{266953356.627944, 581.089945, 2140.473477}},
         {1060.411220, 1100.512812, HyperbolicFunction{289895401.647429, 567.734525, 2108.098250}},
         {1100.512812, 1141.250370, HyperbolicFunction{307187111.879935, 557.345359, 2088.183152}},
         {1141.250370, 1144.398619, HyperbolicFunction{330251.111271, 1081.433645, 2896.870850}},
         {1144.398619, 1149.013667, HyperbolicFunction{1873616.521263, 1032.099116, 2831.603168}},
         {1149.013667, 1160.454977, HyperbolicFunction{2444966.698858, 1021.252369, 2818.886466}},
         {1160.454977, 1178.669793, HyperbolicFunction{3449568.608434, 1004.327988, 2803.545820}},
         {1178.669793, 1181.196292, HyperbolicFunction{20.504422, 1174.880045, 2915.609229}}}};

    PiecewieseDecLinearFunction g{
        {{0.000000, 96.000000, LinearFunction{-33.333333, 0.000000, 4000.000000}}}};
    StatefulPiecewieseDecLinearFunction cf{g};

    std::vector<PiecewieseSolution> solutions;
    compose_minimal(f, cf, std::back_inserter(solutions));

    CHECK(solutions.size() == 1);

    auto & [ delta_1, h_1 ] = solutions[0];
    CHECK(h_1.min_x() == Approx(959.982878));
    CHECK(h_1(h_1.min_x()) == Approx(f(h_1.min_x())));
    CHECK(h_1(h_1.max_x()) == Approx(800).epsilon(0.001));
}
