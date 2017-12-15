#include "ev/phem.hpp"

#include "catch.hpp"

using namespace charge;
using namespace charge::ev;

TEST_CASE("PHEM function fixtures", "phem") {
    auto model = detail::make_consumption_model(detail::PHEMVehicleType::PEUGEOT_ION);

    // uphill
    auto result_0 = model->tradeoff_function(500, 0.1, 50, 100);
    // plane
    auto result_1 = model->tradeoff_function(500, 0, 50, 100);
    // downhill
    auto result_2 = model->tradeoff_function(500, -0.1, 50, 100);
    // this should be a constant function
    auto result_3 = model->tradeoff_function(500, 0, 50, 50.2);

    CHECK(result_0.min_x == Approx(500 / (100 / 3.6)));
    CHECK(result_0.max_x == Approx(500 / (50 / 3.6)));
    CHECK(result_0(result_0.min_x) == Approx(237.6947f));
    CHECK(result_0(result_0.max_x) == Approx(197.00914f));

    CHECK(result_1.min_x == Approx(500 / (100 / 3.6)));
    CHECK(result_1.max_x == Approx(500 / (50 / 3.6)));
    CHECK(result_1(result_1.min_x) == Approx(94.5083f));
    CHECK(result_1(result_1.max_x) == Approx(53.82275f));

    CHECK(result_2.min_x == Approx(500 / (100 / 3.6)));
    CHECK(result_2.max_x == Approx(500 / (50 / 3.6)));
    CHECK(result_2(result_2.min_x) == Approx(-48.6781f));
    CHECK(result_2(result_2.max_x) == Approx(-89.36366f));

    CHECK(result_3.min_x == Approx(500 / (50.2 / 3.6)));
    CHECK(result_3.max_x == result_3.min_x);
    CHECK(result_3(result_3.max_x) == Approx(53.93146));
    CHECK(result_3(result_3.min_x) == Approx(result_3(result_3.max_x)));
}
