#include "common/roots.hpp"

#include <catch.hpp>

namespace Catch {
template <>
inline std::string
toString<std::optional<double>>(const std::optional<double> &d) {
    if (d)
    {
        return std::to_string(*d);
    }
    return "{}";
}
}

using namespace charge;
using namespace charge::common;

TEST_CASE("Check degree 3 roots that would involve complex terms", "[roots]")
{
    auto [x_0, x_1, x_2] = real_roots(1, 0, -15, -4);
    CHECK(x_0 == 4.0);
    CHECK(x_1 == -0.26794919243112153);
    CHECK(x_2 == -3.7320508075688785);
}

TEST_CASE("Check degree 3 roots with one root", "[roots]")
{
    auto [x_0, x_1, x_2] = real_roots(1, -9, 27, -27);
    CHECK(x_0 == 3.0f);
    CHECK(!x_1);
    CHECK(!x_2);
}

TEST_CASE("Check degree 3 roots with double root", "[roots]")
{
    auto [x_0, x_1, x_2] = real_roots(1, 3, 0, 0);
    CHECK(x_0 == 2.2204460492503131e-16);
    CHECK(x_1 == 2.2204460492503131e-16);
    CHECK(x_2 == -3.0);

    auto [x_3, x_4, x_5] = unique_real_roots(1, 3, 0, 0);
    CHECK(x_3 == 2.2204460492503131e-16);
    CHECK(x_4 == -3.0);
    CHECK(!x_5);
}

TEST_CASE("Check degree 4 roots with one quadruple root", "[roots]")
{
    // (x-3)^4
    auto [x_0, x_1, x_2, x_3] = real_roots(1, -12, 54, -108, 81);

    CHECK(x_0 == 3.0f);
    CHECK(x_1 == 3.0f);
    CHECK(x_2 == 3.0f);
    CHECK(x_3 == 3.0f);
}

TEST_CASE("Check degree 4 roots with one double root", "[roots]")
{
    // (x-3)^2(x+5)^2
    auto [x_0, x_1, x_2, x_3] = real_roots(1, 4, -26, -60, 225);

    CHECK(x_0 == 3.0f);
    CHECK(x_1 == -5.0f);
    CHECK(x_2 == 3.0f);
    CHECK(x_3 == -5.0f);
}

TEST_CASE("Check degree 4 roots with three unique roots", "[roots]")
{
    // (x-3)^2(x+5)(x-2)
    auto [x_0, x_1, x_2, x_3] = real_roots(1, -3, -19, 87, -90);

    CHECK(x_0 == 3.0f);
    CHECK(x_1 == 3.0f);
    CHECK(x_2 == 2.0f);
    CHECK(x_3 == -5.0f);
}

TEST_CASE("Check degree 4 roots with two real and two imaginary roots", "[roots]")
{
    // (x^2 + 1)(x+5)(x-2)
    auto [x_0, x_1, x_2, x_3] = real_roots(1, 3, -9, 3, -10);

    CHECK(x_0 == Approx(2.0f));
    CHECK(x_1 == Approx(-5.0f));
    CHECK(!x_2);
    CHECK(!x_3);
}

TEST_CASE("Check degree 3 roots - Regression 1", "[roots]")
{
    auto [x_0, x_1, x_2] = real_roots(1, 19, 99, 81);
    CHECK(x_0 == Approx(-1.0f));
    CHECK(x_1 == Approx(-9.0f));
    CHECK(x_2 == Approx(-9.0f));
}

TEST_CASE("Check degree 3 roots - Regression 2", "[roots]")
{
    auto [x_0, x_1, x_2] = real_roots(8, -262.81100463867188, 2397.2367596132972, -6675.3395470586256);
    CHECK(x_0 == Approx(19.900917));
    CHECK(x_1 == Approx(6.476679));
    CHECK(x_2 == Approx(6.473780));
}

TEST_CASE("Check degree 4 roots - Regression 1", "[roots]")
{
    auto [x_0, x_1, x_2, x_3] = real_roots(1, -2015.6405029296875, 1523294.9936860988, -511562841.94050783, 64412731711.830521);

    CHECK(x_0 == Approx(519.956783));
    CHECK(x_1 == Approx(487.863469));
    CHECK(!x_2);
    CHECK(!x_3);
}
