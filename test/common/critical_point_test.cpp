#include "common/critical_point.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

// We get a critical point that lets us prove there is an intersection
TEST_CASE("Hypbolic functions that intersect twice", "[critical_point]")
{
    HyperbolicFunction f {64, 1, 200};
    HyperbolicFunction g {640, 0.5, 0};

    auto x = critical_point(f, g);

    CHECK(f(1.25) > g(1.25));
    CHECK(f(1.5) < g(1.5));
    CHECK(f(3) > g(3));

    CHECK(x == Approx(1.43311));
}

// We still get a critical point but we already know over the endpoints
// that it intersects
TEST_CASE("Hypbolic functions that intersect once", "[critical_point]")
{
    HyperbolicFunction f {64, 1, 0};
    HyperbolicFunction g {640, 0.25, 100};

    auto x = critical_point(f, g);

    CHECK(f(1.25) > g(1.25));
    CHECK(f(1.5) < g(1.5));
    CHECK(f(3) < g(3));

    CHECK(x == Approx(1.64966));
}

// We get a critical point for hyperbolic and linear with two intersections
TEST_CASE("Hypbolic and linear function that intersect twice", "[critical_point]")
{
    HyperbolicFunction f {64, 1, 300};
    LinearFunction g {-200, 0, 1000};

    auto x = critical_point(f, g);

    CHECK(f(1.25) > g(1.25));
    CHECK(f(1.5) < g(1.5));
    CHECK(f(4) > g(4));

    CHECK(x == Approx(1.86177));
}

// We get a critical point for hyperbolic and linear with one, but it disproves that we intersect
TEST_CASE("Hypbolic and linear function that do not intersect", "[critical_point]")
{
    HyperbolicFunction f {64, 1, 300};
    LinearFunction g {-200, 0, 600};

    auto x = critical_point(f, g);

    CHECK(f(1.25) > g(1.25));
    CHECK(f(1.5) > g(1.5));
    CHECK(f(4) > g(4));

    CHECK(x == Approx(1.86177));
}
