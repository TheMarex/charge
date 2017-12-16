#ifndef CHARGE_EV_PHEM_HPP
#define CHARGE_EV_PHEM_HPP

#include "common/constant_function.hpp"
#include "common/serialization.hpp"

#include "ev/consumption_model.hpp"

#include <boost/algorithm/string.hpp>

#include <cstdint>

namespace charge::ev {
namespace detail {

enum class PHEMVehicleType : std::int32_t {
    PEUGEOT_ION,
    EV_NO_AUX,
    EV_SPRING,
    EV_SUMMER,
    EV_WINTER,
    INVALID = -1
};

class PHEMConsumptionModel : public ConsumptionModel {
  public:
    PHEMConsumptionModel(double speed_parameter, double slope_parameter, double const_parameter)
        : speed_parameter(speed_parameter), slope_parameter(slope_parameter),
          const_parameter(const_parameter) {}

    LimitedTradeoffFunction tradeoff_function(const double length, const double slope,
                                              const double min_speed,
                                              const double max_speed) const override final {
        assert(min_speed <= max_speed);
        assert(min_speed > 0);
        assert(max_speed < 300);

        const double limited_slope = std::max(-10.0, slope*100);
        // Original code has a scaling by 1000, this causes numeric problems.
        // const double a = 1000.0 * length * speed_parameter * (3.6 * 3.6 * length * length);
        // const double b = 0.0;
        // const double c = 1000.0 * length * (slope_parameter * limited_slope + const_parameter);
        const double a = length * speed_parameter * (3.6 * 3.6 * length * length);
        const double b = 0.0;
        const double c = length * (slope_parameter * limited_slope + const_parameter);
        const double min_duration = length * 3.6 / max_speed;
        const double max_duration = length * 3.6 / min_speed;
        assert(min_duration <= max_duration);
        assert(min_duration >= 0);

        assert(std::isfinite(a));
        assert(std::isfinite(b));
        assert(std::isfinite(c));
        assert(std::isfinite(min_duration));
        assert(std::isfinite(max_duration));

        LimitedTradeoffFunction tradeoff;
        if (a > 0)
            tradeoff = {min_duration, max_duration, common::HyperbolicFunction{a, b, c}};
        else
            tradeoff = {min_duration, max_duration, common::ConstantFunction{c}};

        // Only return a tradeoff function of the min and max speed are different
        if (max_speed - min_speed <= 1 || max_duration - min_duration < 1) {
            auto constant_consumption = tradeoff(tradeoff.min_x);
            return LimitedTradeoffFunction{min_duration, min_duration, common::ConstantFunction{constant_consumption}};
        } else {
            return tradeoff;
        }
    }

  private:
    double speed_parameter;
    double slope_parameter;
    double const_parameter;
};

std::unique_ptr<ConsumptionModel> make_consumption_model(PHEMVehicleType type) {
    double speed_parameter, slope_parameter, const_parameter;

    switch (type) {
    case PHEMVehicleType::PEUGEOT_ION:
        speed_parameter = 0.00001084948;
        slope_parameter = 0.02863728;
        const_parameter = 0.08052179;
        break;
    case PHEMVehicleType::EV_NO_AUX:
        speed_parameter = 0.00001129197;
        slope_parameter = 0.05127720;
        const_parameter = 0.1247448;
        break;
    case PHEMVehicleType::EV_SPRING:
        speed_parameter = 0.00001105337;
        slope_parameter = 0.05135052;
        const_parameter = 0.1287824;
        break;
    case PHEMVehicleType::EV_SUMMER:
        speed_parameter = 0.000009192936;
        slope_parameter = 0.05194433;
        const_parameter = 0.1605070;
        break;
    case PHEMVehicleType::EV_WINTER:
        speed_parameter = 0.000004730422;
        slope_parameter = 0.05345339;
        const_parameter = 0.2379125;
        break;
    default:
        throw std::runtime_error("Unknown vehicle type");
    }

    return std::make_unique<PHEMConsumptionModel>(speed_parameter, slope_parameter,
                                                  const_parameter);
}
}

std::unordered_map<std::string, std::unique_ptr<ConsumptionModel>>
make_phem_consumption_models() {
    std::unordered_map<std::string, std::unique_ptr<ConsumptionModel>> models;

    models["Peugeot Ion"] =
        detail::make_consumption_model(detail::PHEMVehicleType::PEUGEOT_ION);
    models["EV No Aux"] =
        detail::make_consumption_model(detail::PHEMVehicleType::EV_NO_AUX);
    models["EV Spring"] =
        detail::make_consumption_model(detail::PHEMVehicleType::EV_SPRING);
    models["EV Summer"] =
        detail::make_consumption_model(detail::PHEMVehicleType::EV_SUMMER);
    models["EV Winter"] =
        detail::make_consumption_model(detail::PHEMVehicleType::EV_WINTER);

    return models;
}
}

#endif
