#include "common/intersection.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Intersect linear functions - one intersection", "[intersect]") {
    LinearFunction lhs_1{-1, 0, 2};
    LinearFunction rhs_1{3, 0, -5};

    auto result_1 = intersection(lhs_1, rhs_1);
    REQUIRE(result_1);
    auto x_1 = *result_1;
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));

    auto lhs_2 = lhs_1;
    LinearFunction rhs_2 = {-0.5, 0, 2};

    auto result_2 = intersection(lhs_2, rhs_2);
    REQUIRE(result_2);
    const auto x_2 = *result_2;
    REQUIRE(lhs_2(x_2) == rhs_2(x_2));

    LinearFunction lhs_3{-1, 4, 2};
    LinearFunction rhs_3{3, 4, -5};

    auto result_3 = intersection(lhs_3, rhs_3);
    REQUIRE(result_3);
    const auto x_3 = *result_3;
    REQUIRE(lhs_3(x_3) == rhs_3(x_3));
}

TEST_CASE("Intersect linear functions - no intersection", "[intersect]") {
    LinearFunction lhs_1{-1, 0, 2};
    LinearFunction rhs_1{-1, 0, -5};

    auto result_1 = intersection(lhs_1, rhs_1);
    REQUIRE(!result_1);

    LinearFunction lhs_2{6, 4, 2};
    LinearFunction rhs_2{6, 3, -5};

    auto result_2 = intersection(lhs_2, rhs_2);
    REQUIRE(!result_1);
}

TEST_CASE("Intersect linear and hyperbolic functions - one intersection", "[intersect]") {
    LinearFunction lhs_1{-1, 0, 3};
    HyperbolicFunction rhs_1{4, 0, 0};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(!results_1[1]);
    const auto x_1 = *results_1[0];
    REQUIRE(x_1 == 2);
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));

    LinearFunction lhs_2{-1, 5, 3};
    HyperbolicFunction rhs_2{4, 5, 0};

    auto results_2 = intersection(lhs_2, rhs_2);
    REQUIRE(results_2[0]);
    REQUIRE(!results_2[1]);
    const auto x_2 = *results_2[0];
    CHECK(x_2 == 7);
    CHECK(lhs_2(x_2) == rhs_2(x_2));
}

TEST_CASE("Intersect linear and hyperbolic functions - two intersection", "[intersect]") {
    LinearFunction lhs_1{-1, 0, 5};
    HyperbolicFunction rhs_1{4, 0, 0};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(results_1[1]);
    const auto x_1_0 = *results_1[0];
    const auto x_1_1 = *results_1[1];
    CHECK(x_1_0 == Approx(4.82843f));
    CHECK(lhs_1(x_1_0) == Approx(rhs_1(x_1_0)));
    CHECK(x_1_1 == Approx(1.0));
    CHECK(lhs_1(x_1_1) == Approx(rhs_1(x_1_1)));

    LinearFunction lhs_2{-1, 5, 5};
    HyperbolicFunction rhs_2{4, 5, 0};

    auto results_2 = intersection(lhs_2, rhs_2);
    REQUIRE(results_2[0]);
    REQUIRE(results_2[1]);
    const auto x_2_0 = *results_2[0];
    const auto x_2_1 = *results_2[1];
    CHECK(x_2_0 == Approx(9.82843f));
    CHECK(lhs_2(x_2_0) == Approx(rhs_2(x_2_0)));
    CHECK(x_2_1 == Approx(6));
    CHECK(lhs_2(x_2_1) == Approx(rhs_2(x_2_1)));
}

TEST_CASE("Intersect linear functions - regression test 1", "[intersect]") {
    LinearFunction lhs_1{-2.5, 0, 7.25};
    LinearFunction rhs_1{-1.5, 0, 4.5};

    auto result_1 = intersection(lhs_1, rhs_1);
    REQUIRE(result_1);
    const auto x_1 = *result_1;
    REQUIRE(x_1 == 2.75);
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));
}

TEST_CASE("Intersect limited linear functions - intersection in valid range", "[intersect]") {
    using LimitedLinearFunction = LimitedFunction<LinearFunction, inf_bound, clamp_bound>;
    LimitedLinearFunction lhs_1{0, 3, LinearFunction{-2.5, 0, 7.25}};
    LimitedLinearFunction rhs_1{1, 4, LinearFunction{-1.5, 0, 4.5}};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(results_1[1]);
    REQUIRE(!results_1[2]);
    const auto x_1 = *results_1[0];
    REQUIRE(x_1 == 2.75);
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));

    const auto x_2 = *results_1[1];
    REQUIRE(x_2 == Approx(3.16667));
    REQUIRE(lhs_1(x_2) == rhs_1(x_2));

    auto results_2 = intersection(rhs_1, lhs_1);
    REQUIRE(results_1[0] == results_2[0]);
    REQUIRE(results_1[1] == results_2[1]);
    REQUIRE(results_1[2] == results_2[2]);
}

TEST_CASE("Intersect limited linear functions - intersection on clamped region", "[intersect]") {
    using LimitedLinearFunction = LimitedFunction<LinearFunction, inf_bound, clamp_bound>;
    LimitedLinearFunction lhs_1{0, 3, LinearFunction{-2.5, 0, 7.25}};
    LimitedLinearFunction rhs_1{1, 2, LinearFunction{-1.5, 0, 4.5}};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(!results_1[1]);
    REQUIRE(!results_1[2]);
    const auto x_1 = *results_1[0];
    REQUIRE(x_1 == Approx(2.3));
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));
}

TEST_CASE("Intersect limited linear functions - intersection before clamped region",
          "[intersect]") {
    using LimitedLinearFunction = LimitedFunction<LinearFunction, inf_bound, clamp_bound>;
    LimitedLinearFunction lhs_1{0, 2, LinearFunction{-2.5, 0, 7.25}};
    LimitedLinearFunction rhs_1{1, 4, LinearFunction{-1.5, 0, 4.5}};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(!results_1[0]);
    REQUIRE(!results_1[1]);
    REQUIRE(!results_1[2]);
}

TEST_CASE("Intersect hyperbolic functions - one intersection", "[intersect]") {
    HyperbolicFunction lhs_1{8, 0, -1};
    HyperbolicFunction rhs_1{4, 0, 0};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(!results_1[1]);
    const auto x_1 = *results_1[0];
    REQUIRE(lhs_1(x_1) == rhs_1(x_1));
}

TEST_CASE("Intersect hyperbolic functions - two intersections", "[intersect]") {
    HyperbolicFunction lhs_1{1, 0.25, 1};
    HyperbolicFunction rhs_1{4, 0, 0};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(results_1[1]);
    const auto x_1 = *results_1[0];
    const auto x_2 = *results_1[1];
    REQUIRE(x_1 == Approx(1.61223f));
    REQUIRE(x_2 == Approx(0.51834f));
    REQUIRE(lhs_1(x_1) == Approx(rhs_1(x_1)));
    REQUIRE(lhs_1(x_2) == Approx(rhs_1(x_2)));
}

TEST_CASE("Intersect hyperbolic functions - regression test 1", "[intersect]") {
    HyperbolicFunction lhs_1{394773248, 19.0788212, 177889.062};
    HyperbolicFunction rhs_1{34161468, 57.9108124, 222909.203};

    auto results_1 = intersection(lhs_1, rhs_1);
    REQUIRE(results_1[0]);
    REQUIRE(results_1[1]);
    const auto x_1 = *results_1[0];
    const auto x_2 = *results_1[1];
    REQUIRE(lhs_1(x_1) == Approx(rhs_1(x_1)));
    REQUIRE(lhs_1(x_2) == Approx(rhs_1(x_2)));
}

//TEST_CASE("Intersect hyperbolic functions - regression test 2", "[intersect]") {
//    HyperbolicFunction lhs_1{792.49469, 397.837799, 1159191.38};
//    HyperbolicFunction rhs_1{375.546844, 397.844452, 1160231.75};
//
//    auto results_1 = intersection(lhs_1, rhs_1);
//    REQUIRE(results_1[0]);
//    REQUIRE(results_1[1]);
//    const auto x_1 = *results_1[0];
//    const auto x_2 = *results_1[1];
//    REQUIRE(lhs_1(x_1) == Approx(rhs_1(x_1)));
//    REQUIRE(lhs_1(x_2) == Approx(rhs_1(x_2)));
//}
