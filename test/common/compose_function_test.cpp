#include "common/compose_functions.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Compose linear function with stateful function", "[compose_function]")
{
    HypOrLinFunction f = LinearFunction{-0.5, 0};

    PiecewieseDecLinearFunction pwf {{
        {0, 2, LinearFunction(-1, 0)},
        {2, 7, LinearFunction(-0.5, -1)},
        {7, 10, LinearFunction(-0.25, -2.75)},
    }};

    REQUIRE(pwf(1.99999) == Approx(pwf(2.00001)));
    REQUIRE(pwf(6.99999) == Approx(pwf(7.00001)));

    StatefulPiecewieseDecLinearFunction g { pwf };

    auto result = compose_function(ConstantFunction {1}, f, g);
    REQUIRE(f(1.0) == -0.5);
    REQUIRE(pwf(0.5) == -0.5);
    REQUIRE(result(1.0) == pwf(0.5));
    REQUIRE(result(2.0) == pwf(1.5));
    REQUIRE(result(9.0) == pwf(8.5));
}
