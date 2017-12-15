#include "ev/battery.hpp"
#include "ev/charging_model.hpp"

#include "../helper/function_comparator.hpp"
#include "../helper/function_printer.hpp"

#include "catch.hpp"

using namespace charge;
using namespace charge::ev;

using PartialChargingFunction =
    common::LimitedFunction<common::LinearFunction, common::inf_bound, common::clamp_bound>;

TEST_CASE("Charging model for Peugeou", "phem") {
    auto model = ChargingModel(BATTERY_PEUGEOT_ION);

    auto reference_1 =
        PartialChargingFunction{0, 2111.24706561, common::LinearFunction{-6.06276745556, 0, 16000}};
    auto reference_2 = PartialChargingFunction{
        2111.24706561, 2262.2117708, common::LinearFunction{-5.29925189444, 2111.24706561, 3200}};
    auto reference_3 = PartialChargingFunction{
        2262.2117708, 2468.14118362, common::LinearFunction{-3.88482630556, 2262.2117708, 2400}};
    auto reference_4 = PartialChargingFunction{
        2468.14118362, 2771.20000364, common::LinearFunction{-2.63975158333, 2468.14118362, 1600}};
    auto reference_5 = PartialChargingFunction{
        2771.20000364, 3669.08234813, common::LinearFunction{-0.890985333333, 2771.20000364, 800}};

    const auto result_standart_idx = model.get_index(3700);
    const auto result_accelerated_idx = model.get_index(22000);
    const auto result_quick_idx = model.get_index(120000);

    const auto &result_standart = model[result_standart_idx].function.functions;
    const auto &result_accelerated = model[result_accelerated_idx].function.functions;
    const auto &result_quick = model[result_quick_idx].function.functions;

    REQUIRE(result_accelerated.size() == 5);
    CHECK(ApproxFunction(reference_1) == result_accelerated[0]);
    CHECK(ApproxFunction(reference_2) == result_accelerated[1]);
    CHECK(ApproxFunction(reference_3) == result_accelerated[2]);
    CHECK(ApproxFunction(reference_4) == result_accelerated[3]);
    CHECK(ApproxFunction(reference_5) == result_accelerated[4]);

    REQUIRE(result_standart.size() == 5);
    CHECK(result_standart[0](result_standart[0].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.0)));
    CHECK(result_standart[0](result_standart[0].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.8)));
    CHECK(result_standart[1](result_standart[1].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.8)));
    CHECK(result_standart[1](result_standart[1].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.85)));
    CHECK(result_standart[2](result_standart[2].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.85)));
    CHECK(result_standart[2](result_standart[2].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.90)));
    CHECK(result_standart[3](result_standart[3].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.90)));
    CHECK(result_standart[3](result_standart[3].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.95)));
    CHECK(result_standart[4](result_standart[4].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.95)));
    CHECK(result_standart[4](result_standart[4].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 1.0)));

    CHECK(result_accelerated[0](result_accelerated[0].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.0)));
    CHECK(result_accelerated[0](result_accelerated[0].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.8)));
    CHECK(result_accelerated[1](result_accelerated[1].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.8)));
    CHECK(result_accelerated[1](result_accelerated[1].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.85)));
    CHECK(result_accelerated[2](result_accelerated[2].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.85)));
    CHECK(result_accelerated[2](result_accelerated[2].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.90)));
    CHECK(result_accelerated[3](result_accelerated[3].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.90)));
    CHECK(result_accelerated[3](result_accelerated[3].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.95)));
    CHECK(result_accelerated[4](result_accelerated[4].min_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.95)));
    CHECK(result_accelerated[4](result_accelerated[4].max_x) ==
          Approx(BATTERY_PEUGEOT_ION * (1.0 - 1.0)).epsilon(0.25));

    REQUIRE(result_quick.size() == 1);
    REQUIRE(result_quick[0](result_quick[0].min_x) == Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.0)));
    REQUIRE(result_quick[0](result_quick[0].max_x) == Approx(BATTERY_PEUGEOT_ION * (1.0 - 0.8)));
}

TEST_CASE("Charging model for Tesla Model X", "phem") {
    auto model = ChargingModel(BATTERY_TESLA_MODEL_X);

    auto reference_1 =
        PartialChargingFunction{0, 11216.000036, common::LinearFunction{-6.06276745556, 0, 85000}};
    auto reference_2 = PartialChargingFunction{
        11216.000036, 12018.0000324, common::LinearFunction{-5.29925189444, 11216.000036, 17000}};
    auto reference_3 = PartialChargingFunction{
        12018.0000324, 13112.000038, common::LinearFunction{-3.88482630556, 12018.0000324, 12750}};
    auto reference_4 = PartialChargingFunction{
        13112.000038, 14722.0000194, common::LinearFunction{-2.63975158333, 13112.000038, 8500}};
    auto reference_5 = PartialChargingFunction{
        14722.0000194, 19491.9999745, common::LinearFunction{-0.890985333333, 14722.0000194, 4250}};

    const auto result_standart_idx = model.get_index(3700);
    const auto result_accelerated_idx = model.get_index(22000);
    const auto result_quick_idx = model.get_index(120000);

    const auto &result_standart = model[result_standart_idx].function.functions;
    const auto &result_accelerated = model[result_accelerated_idx].function.functions;
    const auto &result_quick = model[result_quick_idx].function.functions;

    REQUIRE(result_accelerated.size() == 5);
    CHECK(ApproxFunction(reference_1) == result_accelerated[0]);
    CHECK(ApproxFunction(reference_2) == result_accelerated[1]);
    CHECK(ApproxFunction(reference_3) == result_accelerated[2]);
    CHECK(ApproxFunction(reference_4) == result_accelerated[3]);
    CHECK(ApproxFunction(reference_5) == result_accelerated[4]);

    REQUIRE(result_standart.size() == 5);
    CHECK(result_standart[0](result_standart[0].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.0)));
    CHECK(result_standart[0](result_standart[0].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.8)));
    CHECK(result_standart[1](result_standart[1].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.8)));
    CHECK(result_standart[1](result_standart[1].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.85)));
    CHECK(result_standart[2](result_standart[2].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.85)));
    CHECK(result_standart[2](result_standart[2].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.90)));
    CHECK(result_standart[3](result_standart[3].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.90)));
    CHECK(result_standart[3](result_standart[3].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.95)));
    CHECK(result_standart[4](result_standart[4].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.95)));
    CHECK(result_standart[4](result_standart[4].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 1.0)).epsilon(0.001));

    CHECK(result_accelerated[0](result_accelerated[0].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.0)));
    CHECK(result_accelerated[0](result_accelerated[0].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.8)));
    CHECK(result_accelerated[1](result_accelerated[1].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.8)));
    CHECK(result_accelerated[1](result_accelerated[1].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.85)));
    CHECK(result_accelerated[2](result_accelerated[2].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.85)));
    CHECK(result_accelerated[2](result_accelerated[2].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.90)));
    CHECK(result_accelerated[3](result_accelerated[3].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.90)));
    CHECK(result_accelerated[3](result_accelerated[3].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.95)));
    CHECK(result_accelerated[4](result_accelerated[4].min_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.95)));
    CHECK(result_accelerated[4](result_accelerated[4].max_x) ==
          Approx(BATTERY_TESLA_MODEL_X * (1.0 - 1.0)));
    CHECK(result_accelerated[0]->deriv(result_accelerated[0].min_x) == Approx(-6.0627674556));
    CHECK((result_accelerated[0].max_x - result_accelerated[0].min_x) == Approx(11216));

    CHECK(result_quick.size() == 1);
    CHECK(result_quick[0](result_quick[0].min_x) == Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.0)));
    CHECK(result_quick[0](result_quick[0].max_x) == Approx(BATTERY_TESLA_MODEL_X * (1.0 - 0.8)));
    CHECK(result_quick[0]->deriv(result_quick[0].min_x) == Approx(-33.3333333333));
    CHECK((result_quick[0].max_x - result_quick[0].min_x) == Approx(2040));
}
