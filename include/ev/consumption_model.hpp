#ifndef CHARGE_CONSUMPTION_MODEL_HPP
#define CHARGE_CONSUMPTION_MODEL_HPP

#include "common/hyp_lin_function.hpp"
#include "common/limited_function.hpp"

#include "ev/limited_tradeoff_function.hpp"

namespace charge::ev {

class ConsumptionModel {
  public:
    virtual ~ConsumptionModel() = default;

    virtual LimitedTradeoffFunction tradeoff_function(const double length, const double slope,
                                                      const double min_speed,
                                                      const double max_speed) const = 0;
};
}

#endif
