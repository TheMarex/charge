#include "common/minimize_combined_function.hpp"

#include "../helper/function_comparator.hpp"
#include "../helper/function_printer.hpp"

#include <catch.hpp>

#include <vector>

using namespace charge;
using namespace charge::common;

namespace {
template <typename FunctionT>
auto make_sampled_link(const PiecewieseDecHypOrLinFunction &f, const FunctionT &g) {
    // The result has to be the minimum of all linked pairs
    std::vector<LimitedHypOrLinFunction> values;
    for (const auto &function : f.functions)
        for (const auto solution : combine_minimal(function, g))
            if (solution)
                values.push_back(std::get<1>(*solution));

    auto max_x = g.max_x + f.max_x();
    auto min_x = g.min_x + f.min_x();
    return Samples{values, [](auto lhs, auto rhs) { return std::min<double>(lhs, rhs); }, min_x,
                   max_x};
}
}

TEST_CASE("Constant and Constant", "[minimize combined]") {
    auto[first, second, third] = combine_minimal(LimitedLinearFunction{1, 1, ConstantFunction{5}},
                                                 LimitedLinearFunction{1, 1, ConstantFunction{5}});
    auto reference_1_first = DeltaAndOptimalFunction{
        ConstantFunction{1}, LimitedHypOrLinFunction{2, 2, ConstantFunction{10}}};

    REQUIRE(first);
    REQUIRE(!second);
    REQUIRE(!third);

    REQUIRE(*first == reference_1_first);
}

TEST_CASE("Linear and Constant", "[minimize combined]") {
    auto[first, second, third] = combine_minimal(LimitedLinearFunction{1, 2, LinearFunction{-1, 4}},
                                                 LimitedLinearFunction{1, 1, ConstantFunction{5}});
    auto reference_1_first = DeltaAndOptimalFunction{
        LinearFunction{1, -1}, LimitedHypOrLinFunction{2, 3, LinearFunction{-1, 1, 9}}};

    REQUIRE(first);
    REQUIRE(!second);
    REQUIRE(!third);

    REQUIRE(*first == reference_1_first);
}

TEST_CASE("Constant and Linear", "[minimize combined]") {
    auto[first, second, third] =
        combine_minimal(LimitedLinearFunction{1, 1, ConstantFunction{5}},
                        LimitedLinearFunction{1, 2, LinearFunction{-1, 4}});
    auto reference_1_first = DeltaAndOptimalFunction{
        ConstantFunction{1}, LimitedHypOrLinFunction{2, 3, LinearFunction{-1, 1, 9}}};

    REQUIRE(first);
    REQUIRE(!second);
    REQUIRE(!third);

    REQUIRE(*first == reference_1_first);
}

TEST_CASE("Linear and Linear", "[minimize combined]") {
    const auto f_1 = LimitedLinearFunction{1, 2, LinearFunction{-1, 4}};
    const auto g_1 = LimitedLinearFunction{2, 4, LinearFunction{-0.5, 5}};

    REQUIRE(f_1(1) == 3);
    REQUIRE(f_1(2) == 2);
    REQUIRE(g_1(2) == 4);
    REQUIRE(g_1(4) == 3);

    // f is steeper then g
    auto[first_1, second_1, third_1] = combine_minimal(f_1, g_1);
    auto reference_1_first = DeltaAndOptimalFunction{
        LinearFunction{1, -2}, LimitedHypOrLinFunction{3, 4, LinearFunction{-1, 2, 8}}};
    auto reference_1_second = DeltaAndOptimalFunction{
        ConstantFunction{2}, LimitedHypOrLinFunction{4, 6, LinearFunction{-0.5, 2, 7}}};

    REQUIRE(std::get<1>(reference_1_first)(3) == 7);
    REQUIRE(std::get<1>(reference_1_first)(4) == 6);
    REQUIRE(std::get<1>(reference_1_second)(4) == 6);
    REQUIRE(std::get<1>(reference_1_second)(6) == 5);

    REQUIRE(first_1);
    REQUIRE(second_1);
    REQUIRE(!third_1);

    REQUIRE(*first_1 == reference_1_first);
    REQUIRE(*second_1 == reference_1_second);

    // g is steeper then f
    auto f_2 = g_1;
    auto g_2 = f_1;
    auto[first_2, second_2, third_2] = combine_minimal(f_2, g_2);
    auto reference_2_first = DeltaAndOptimalFunction{
        ConstantFunction{2}, LimitedHypOrLinFunction{3, 4, LinearFunction{-1, 2, 8}}};
    auto reference_2_second = DeltaAndOptimalFunction{
        LinearFunction{1, -2}, LimitedHypOrLinFunction{4, 6, LinearFunction{-0.5, 2, 7}}};

    REQUIRE(std::get<1>(reference_2_first)(3) == 7);
    REQUIRE(std::get<1>(reference_2_first)(4) == 6);
    REQUIRE(std::get<1>(reference_2_second)(4) == 6);
    REQUIRE(std::get<1>(reference_2_second)(6) == 5);

    REQUIRE(first_2);
    REQUIRE(second_2);
    REQUIRE(!third_2);

    REQUIRE(*first_2 == reference_2_first);
    REQUIRE(*second_2 == reference_2_second);
}

TEST_CASE("Hyperbolic then Linear", "[minimize combined]") {
    const auto f_1 = LimitedHyperbolicFunction{1, 4, HyperbolicFunction{4, 0, -1}};
    const auto g_1 = LimitedLinearFunction{2, 4, LinearFunction{-1 / 32.0f, 2}};

    REQUIRE(f_1(1) == 3);
    REQUIRE(f_1(4) == Approx(-3.0f / 4.0f));
    REQUIRE(g_1(2) == Approx(62.0f / 32.0f));
    REQUIRE(g_1(4) == Approx(60.0f / 32.0f));

    auto[first_1, second_1, third_1] = combine_minimal(f_1, g_1);
    auto reference_1_first = DeltaAndOptimalFunction{
        LinearFunction{1, -2},
        LimitedHypOrLinFunction{3, 6, HyperbolicFunction{4, 2, -1 + 62.0f / 32.0f}}};
    auto reference_1_second = DeltaAndOptimalFunction{
        ConstantFunction{4}, LimitedHypOrLinFunction{6, 8, LinearFunction{-1 / 32.0f, 4, 1.25}}};

    REQUIRE(std::get<1>(reference_1_first)(3) == (f_1(1) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_first)(6) == (f_1(4) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_second)(6) == (f_1(4) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_second)(8) == (f_1(4) + g_1(4)));

    REQUIRE(first_1);
    REQUIRE(second_1);
    REQUIRE(!third_1);

    REQUIRE(*first_1 == reference_1_first);
    REQUIRE(*second_1 == reference_1_second);

    auto f_2 = g_1;
    auto g_2 = f_1;
    auto[first_2, second_2, third_2] = combine_minimal(f_2, g_2);
    auto reference_2_first = DeltaAndOptimalFunction{
        ConstantFunction{2},
        LimitedHypOrLinFunction{3, 6, HyperbolicFunction{4, 2, -1 + 62.0f / 32.0f}}};
    auto reference_2_second = DeltaAndOptimalFunction{
        LinearFunction{1, -4}, LimitedHypOrLinFunction{6, 8, LinearFunction{-1 / 32.0f, 4, 1.25}}};

    REQUIRE(std::get<1>(reference_2_first)(3) == (f_2(2) + g_2(1)));
    REQUIRE(std::get<1>(reference_2_first)(6) == (f_2(2) + g_2(4)));
    REQUIRE(std::get<1>(reference_2_second)(6) == (f_2(2) + g_2(4)));
    REQUIRE(std::get<1>(reference_2_second)(8) == (f_2(4) + g_2(4)));

    REQUIRE(first_2);
    REQUIRE(second_2);
    REQUIRE(!third_2);

    REQUIRE(*first_2 == reference_2_first);
    REQUIRE(*second_2 == reference_2_second);
}

TEST_CASE("Linear then Hyperbolic", "[minimize combined]") {
    const auto f_1 = LimitedHyperbolicFunction{1, 4, HyperbolicFunction{4, 0, -1}};
    const auto g_1 = LimitedLinearFunction{2, 4, LinearFunction{-10, 40}};

    REQUIRE(f_1(1) == 3);
    REQUIRE(f_1(4) == Approx(-3.0f / 4.0f));
    REQUIRE(g_1(2) == Approx(20));
    REQUIRE(g_1(4) == Approx(0));

    auto[first_1, second_1, third_1] = combine_minimal(f_1, g_1);
    auto reference_1_first = DeltaAndOptimalFunction{
        ConstantFunction{1}, LimitedHypOrLinFunction{3, 5, LinearFunction{-10, 1, 43}}};
    auto reference_1_second = DeltaAndOptimalFunction{
        LinearFunction{1, -4}, LimitedHypOrLinFunction{5, 8, HyperbolicFunction{4, 4, -1}}};

    REQUIRE(std::get<1>(reference_1_first)(3) == (f_1(1) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_first)(5) == (f_1(1) + g_1(4)));
    REQUIRE(std::get<1>(reference_1_second)(5) == (f_1(1) + g_1(4)));
    REQUIRE(std::get<1>(reference_1_second)(8) == (f_1(4) + g_1(4)));

    REQUIRE(first_1);
    REQUIRE(second_1);
    REQUIRE(!third_1);

    REQUIRE(*first_1 == reference_1_first);
    REQUIRE(*second_1 == reference_1_second);

    auto f_2 = g_1;
    auto g_2 = f_1;
    auto[first_2, second_2, third_2] = combine_minimal(f_2, g_2);
    auto reference_2_first = DeltaAndOptimalFunction{
        LinearFunction{1, -1}, LimitedHypOrLinFunction{3, 5, LinearFunction{-10, 1, 43}}};
    auto reference_2_second = DeltaAndOptimalFunction{
        ConstantFunction{4}, LimitedHypOrLinFunction{5, 8, HyperbolicFunction{4, 4, -1}}};

    REQUIRE(std::get<1>(reference_2_first)(3) == (f_2(2) + g_2(1)));
    REQUIRE(std::get<1>(reference_2_first)(5) == (f_2(4) + g_2(1)));
    REQUIRE(std::get<1>(reference_2_second)(5) == (f_2(4) + g_2(1)));
    REQUIRE(std::get<1>(reference_2_second)(8) == (f_2(4) + g_2(4)));

    REQUIRE(first_2);
    REQUIRE(second_2);
    REQUIRE(!third_2);

    REQUIRE(*first_2 == reference_2_first);
    REQUIRE(*second_2 == reference_2_second);
}

TEST_CASE("Hyperbolic then Linear then Hyperbolic", "[minimize combined]") {
    const auto f_1 = LimitedHyperbolicFunction{1, 4, HyperbolicFunction{4, 0, -1}};
    const auto g_1 = LimitedLinearFunction{2, 4, LinearFunction{-1, 10}};

    REQUIRE(f_1(1) == 3);
    REQUIRE(f_1(4) == Approx(-3.0f / 4.0f));
    REQUIRE(g_1(2) == Approx(8));
    REQUIRE(g_1(4) == Approx(6));

    auto[first_1, second_1, third_1] = combine_minimal(f_1, g_1);
    auto reference_1_first = DeltaAndOptimalFunction{
        LinearFunction{1, -2}, LimitedHypOrLinFunction{3, 4, HyperbolicFunction{4, 2, 7}}};
    auto reference_1_second = DeltaAndOptimalFunction{
        ConstantFunction{2}, LimitedHypOrLinFunction{4, 6, LinearFunction{-1, 2, 10}}};
    auto reference_1_third = DeltaAndOptimalFunction{
        LinearFunction{1, -4}, LimitedHypOrLinFunction{6, 8, HyperbolicFunction{4, 4, 5}}};

    REQUIRE(std::get<1>(reference_1_first)(3) == (f_1(1) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_first)(4) == (f_1(2) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_second)(4) == (f_1(2) + g_1(2)));
    REQUIRE(std::get<1>(reference_1_second)(6) == (f_1(2) + g_1(4)));
    REQUIRE(std::get<1>(reference_1_third)(6) == (f_1(2) + g_1(4)));
    REQUIRE(std::get<1>(reference_1_third)(8) == (f_1(4) + g_1(4)));

    REQUIRE(first_1);
    REQUIRE(second_1);
    REQUIRE(third_1);

    REQUIRE(*first_1 == reference_1_first);
    REQUIRE(*second_1 == reference_1_second);
    REQUIRE(*third_1 == reference_1_third);

    auto f_2 = g_1;
    auto g_2 = f_1;
    auto[first_2, second_2, third_2] = combine_minimal(f_2, g_2);
    auto reference_2_first = DeltaAndOptimalFunction{
        ConstantFunction{2}, LimitedHypOrLinFunction{3, 4, HyperbolicFunction{4, 2, 7}}};
    auto reference_2_second = DeltaAndOptimalFunction{
        LinearFunction{1, -2}, LimitedHypOrLinFunction{4, 6, LinearFunction{-1, 2, 10}}};
    auto reference_2_third = DeltaAndOptimalFunction{
        ConstantFunction{4}, LimitedHypOrLinFunction{6, 8, HyperbolicFunction{4, 4, 5}}};

    REQUIRE(std::get<1>(reference_2_first)(3) == (f_2(2) + g_2(1)));
    REQUIRE(std::get<1>(reference_2_first)(4) == (f_2(2) + g_2(2)));
    REQUIRE(std::get<1>(reference_2_second)(4) == (f_2(2) + g_2(2)));
    REQUIRE(std::get<1>(reference_2_second)(6) == (f_2(4) + g_2(2)));
    REQUIRE(std::get<1>(reference_2_third)(6) == (f_2(4) + g_2(2)));
    REQUIRE(std::get<1>(reference_2_third)(8) == (f_2(4) + g_2(4)));

    REQUIRE(first_2);
    REQUIRE(second_2);
    REQUIRE(third_2);

    REQUIRE(*first_2 == reference_2_first);
    REQUIRE(*second_2 == reference_2_second);
    REQUIRE(*third_2 == reference_2_third);
}

TEST_CASE("Regression test 1 combining hyperbolic functions", "[minimize combined]") {
    LimitedHyperbolicFunction f{26.4031143, 52.8062286,
                                HyperbolicFunction{606280.562, 19.0788212, 77400.2812}};
    LimitedHyperbolicFunction g{3.2385006, 3.45440054,
                                HyperbolicFunction{52409.2969, 0, 5938.07227}};

    auto[first, second, third] = combine_minimal(f, g);
    auto reference_first_delta = LinearFunction{0.693404936095, -8.43586784657, 0};
    auto reference_first_h = LimitedHypOrLinFunction{
        29.6416149, 30.3458014834, HyperbolicFunction{1818498.12571, 19.0788212, 83338.35347}};
    auto reference_second_delta = LinearFunction{1, -3.4544005394};
    auto reference_second_h = LimitedHypOrLinFunction{
        30.3458014834, 56.26062914, HyperbolicFunction{606280.562, 22.53322174, 87730.359702}};

    REQUIRE(first);
    REQUIRE(second);
    CHECK(!third);

    auto & [ first_delta, first_h ] = *first;
    auto & [ second_delta, second_h ] = *second;

    CHECK(ApproxFunction(first_h) == reference_first_h);
    CHECK(ApproxFunction(second_h) == reference_second_h);
    CHECK(ApproxFunction(first_delta) == reference_first_delta);
    CHECK(ApproxFunction(second_delta) == reference_second_delta);
}

TEST_CASE("Regression test 2 combining hyperbolic functions", "[minimize combined]") {
    LimitedHyperbolicFunction f{6, 8, HyperbolicFunction{4.000000, 4.000000, 5.000000}};
    LimitedHyperbolicFunction g{3.15443468, 4, HyperbolicFunction{5, 1, -1}};
    auto[first, second, third] = combine_minimal(f, g);

    DeltaAndOptimalFunction reference_first{
        {0.481413304806, 0, 1.59293353558},
        {9.1544342041, 10.7849531174, HyperbolicFunction{35.8513755798, 5, 4}}};
    DeltaAndOptimalFunction reference_second{
        {1, 0, -4}, {10.7849531174, 12, HyperbolicFunction{4, 8, 4.55555534363}}};

    CHECK(ApproxFunction(std::get<0>(reference_first)) == std::get<0>(*first));
    CHECK(ApproxFunction(std::get<0>(reference_second)) == std::get<0>(*second));
    CHECK(!third);

    auto & [ delta_first, h_first ] = *first;
    auto & [ delta_second, h_second ] = *second;
    // Removed invalid dereference of empty optional 'third'

    CHECK(h_first(9.1545) == Approx(f(6) + g(3.15443468)));
    CHECK(h_first(10.785) == Approx(f(6 + (4 - 3.15443468)) + g(4)).epsilon(0.05));
    CHECK(h_second(10.785) == Approx(h_first(10.785)));
    CHECK(h_second(12) == Approx(f(8) + g(4)));
}

TEST_CASE("Combine piecewiese function with linear function", "[minimized combined]") {
    auto f =
        PiecewieseDecHypOrLinFunction{{LimitedHypOrLinFunction{3, 4, HyperbolicFunction{4, 2, 7}},
                                       LimitedHypOrLinFunction{4, 6, LinearFunction{-1, 2, 10}},
                                       LimitedHypOrLinFunction{6, 8, HyperbolicFunction{4, 4, 5}}}};

    auto g = LimitedHypOrLinFunction{1, 2, LinearFunction{-0.5, 2}};

    auto[delta, result] = combine_minimal(f, g);

    CHECK(result(4) == f(3) + g(1));
    CHECK(result(7.51984214783) == Approx(f(6.51984214783) + g(1)));
    CHECK(result(8.51984214783) == Approx(f(6.51984214783) + g(2)));
    CHECK(result(10) == f(8) + g(2));

    PiecewieseDecHypOrLinFunction reference{
        {{4, 5, HyperbolicFunction{4, 3, 8.5}},
         {5, 7, LinearFunction{-1, 3, 11.5}},
         {7, 7.51984209979, HyperbolicFunction{4, 5, 6.5}},
         {7.51984209979, 8.51984209979, LinearFunction{-0.5, 6.51984209979, 7.62996052495}},
         {8.51984209979, 10, HyperbolicFunction{4, 6, 6}}}};

    CHECK(ApproxFunction(result) == reference);
}

TEST_CASE("Combine piecewiese function with hyperbolic function", "[minimized combined]") {
    auto f =
        PiecewieseDecHypOrLinFunction{{LimitedHypOrLinFunction{3, 4, HyperbolicFunction{4, 2, 7}},
                                       LimitedHypOrLinFunction{4, 6, LinearFunction{-1, 2, 10}},
                                       LimitedHypOrLinFunction{6, 8, HyperbolicFunction{4, 4, 5}}}};

    auto g = LimitedHypOrLinFunction{1.5, 4, HyperbolicFunction{5, 1, -1}};

    auto[delta, result] = combine_minimal(f, g);

    CHECK(result(5) == f(3) + g(2));
    CHECK(result(12) == f(8) + g(4));

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    PiecewieseDecHypOrLinFunction reference{
        {{4.5, 5.07721734502, HyperbolicFunction{5, 4, 10}},
         {5.07721734502, 7.15443469003, HyperbolicFunction{35.851374641, 3, 6}},
         {7.15443469003, 9.15443469003, LinearFunction{-1, 5.15443469003, 10.077217345}},
         {9.15443469003, 10.7849533002, HyperbolicFunction{35.851374641, 5, 4}},
         {10.7849533002, 12, HyperbolicFunction{4, 8, 4.55555555556}}}};

    CHECK(ApproxFunction(result) == reference);
}

TEST_CASE("Combine piecewise function with constant functions - Regression 1",
          "[mimimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{85.6792755, 137.086823, HyperbolicFunction{970518.25, 0, 153.799026}},
         {137.086838, 144.371521, HyperbolicFunction{491.500763, 126.159782, 201.325714}}}};
    LimitedHypOrLinFunction g{0.359484255, 0.359484255, LinearFunction{0, 0, 1.19794917}};

    auto[delta, result] = combine_minimal(f, g);

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    PiecewieseDecHypOrLinFunction reference{
        {{86.038759755, 137.446307255, HyperbolicFunction{970518.25, 0.359484255, 154.99697517}},
         {137.446307255, 144.731005255,
          HyperbolicFunction{491.500763, 126.519266255, 202.523669937}}}};

    CHECK(ApproxFunction(result) == reference);

    CHECK(result(85.6792755 + 0.359484255) == Approx(f(85.6792755) + g(0.359484255)).epsilon(0.01));
    CHECK(result(144.371521 + 0.359484255) == Approx(f(144.371521) + g(0.359484255)).epsilon(0.01));
}

TEST_CASE("Combine piecewise function with hyperbolic functions - Regression 2",
          "[mimimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{116.219994, 185.951981, HyperbolicFunction{2422250, 0, 210.423752}},
         {185.951981, 194.412872, HyperbolicFunction{770.083923, 173.260651, 275.694183}}}};

    LimitedHypOrLinFunction g{4.05287504, 5.47138119,
                              HyperbolicFunction{106.623055, 0, 7.42866898}};

    auto[delta, result] = combine_minimal(f, g);

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    PiecewieseDecHypOrLinFunction reference{
      {{120.27286904, 120.323529874, HyperbolicFunction{106.623055, 116.219994, 397.18420429}},
          {120.323529874, 160.431373892, HyperbolicFunction{2687992.86074, 0, 217.85242098}},
          {160.431373892, 191.42336219, HyperbolicFunction{2422250, 5.47138119, 221.414119664}},
          {191.42336219, 199.88425319, HyperbolicFunction{770.083923, 178.73203219, 286.684549124}}}};


    CHECK(ApproxFunction(result) == reference);

    CHECK(result(120.323532104) == Approx(f(116.219994) + g(4.05287504)).epsilon(0.01));
    CHECK(result(199.884246826) == Approx(f(194.412872) + g(5.47138119)).epsilon(0.01));
}

TEST_CASE("Combine piecewise function with mixed function - Regression 3", "[minimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{493.109589, 583.226318, LinearFunction{-2.640265, 280.226288, 1658.382690}},
         {583.226318, 585.762878, HyperbolicFunction{259.220337, 577.414001, 850.709473}},
         {585.762878, 1483.762817, LinearFunction{-0.890869, 585.762878, 854.428345}},
         {1483.762817, 1490.128052, HyperbolicFunction{259.220337, 1475.413940, 50.709328}}}};

    LimitedHypOrLinFunction g{3.081188, 8.216503,
                              HyperbolicFunction{45.136845, 0.000000, 5.370210}};

    auto[delta, result] = combine_minimal(f, g);

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    CHECK(result(493.109589 + 3.081188) == Approx(f(493.109589) + g(3.081188)).epsilon(0.01));
    CHECK(result(1490.128052 + 8.216503) == Approx(f(1490.128052) + g(8.216503)).epsilon(0.01));
}

TEST_CASE("Combine piecewise function with mixed function - Regression 4", "[minimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{130.881363, 174.617477, HyperbolicFunction{3465958.500000, 0.000000, 236.489670}},
         {174.617477, 207.775497, HyperbolicFunction{2966447.500000, 8.827291, 242.235962}}}};

    LimitedHypOrLinFunction g{15.7005501, 21.1957417,
                              HyperbolicFunction{6198.777832, 0.000000, 28.760328}};

    auto[delta, result] = combine_minimal(f, g);

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    CHECK(result(130.881363 + 15.7005501) == Approx(f(130.881363) + g(15.7005501)).epsilon(0.01));
    CHECK(result(174.617477 + 21.1957417) == Approx(f(174.617477) + g(21.1957417)).epsilon(0.01));
}

TEST_CASE("Combine piecewise function with mixed function - Regression 5", "[minimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{233.764511, 234.813995, HyperbolicFunction{1452200.000000, 136.976501, 634.915649}},
         {234.813995, 235.193710, HyperbolicFunction{19263706.000000, 3.211616, 427.494537}},
         {235.193710, 313.591583, HyperbolicFunction{20074908.000000, 0.000000, 422.538910}},
         {313.591583, 345.440002, HyperbolicFunction{2628659.000000, 154.349472, 523.015625}},
         {345.440002, 353.900879, HyperbolicFunction{770.083923, 332.748657, 590.221863}}}};

    LimitedHypOrLinFunction g{4.046768, 6.474829,
                              HyperbolicFunction{102.258965, 0.000000, 7.097992}};

    auto[delta, result] = combine_minimal(f, g);

    auto samples = make_sampled_link(f, g);
    CHECK(samples == result);

    CHECK(result(233.764511 + 4.046768) == Approx(f(233.764511) + g(4.046768)).epsilon(0.01));
    CHECK(result(353.900879 + 6.474829) == Approx(f(353.900879) + g(6.474829)).epsilon(0.01));
}

TEST_CASE("Combine piecewise wiht constant function - Regression 6", "[minimized combined]") {
    PiecewieseDecHypOrLinFunction f{
        {{1, 1.25, LinearFunction{-400.000000, -2.750000, 2000.000000}},
         {1.25, 9.25, LinearFunction{-50.000000, 1.250000, 400.000000}}}};
    LimitedHypOrLinFunction g{1, 1, ConstantFunction{100}};

    auto[delta, result] = combine_minimal(f, g);

    CHECK(result(10.25) == 100);
}

TEST_CASE("Linear and Linear with same slope", "[minimize combined]") {
    const auto f = LimitedLinearFunction{1, 2, LinearFunction{-1.5, 4}};
    const auto g = LimitedLinearFunction{2, 4, LinearFunction{-1.5, 5}};

    // f is steeper then g
    auto[first, second, third] = combine_minimal(f, g);

    CHECK(first);
    CHECK(!second);
    CHECK(!third);

    auto [delta, h] = *first;

    CHECK(h(f.min_x + g.min_x) == Approx(f(f.min_x) + g(g.min_x)));
    CHECK(h(f.max_x + g.min_x) == Approx(f(f.max_x) + g(g.min_x)));
    CHECK(h(f.max_x + g.max_x) == Approx(f(f.max_x) + g(g.max_x)));
    CHECK(h(f.max_x + g.max_x + 10) == Approx(f(f.max_x) + g(g.max_x)));

    CHECK(delta(f.min_x + g.min_x) == Approx(f.min_x));
    CHECK(delta(f.max_x + g.max_x) == Approx(f.max_x));
}

