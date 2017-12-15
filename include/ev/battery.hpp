#ifndef CHARGE_EV_BATTERY_HPP
#define CHARGE_EV_BATTERY_HPP

#include <cmath>
#include <cstdint>

namespace charge::ev {
constexpr double kWh_Wh = 1e3f; // kWh -> Wh
constexpr double BATTERY_TESLA_MODEL_X = 85 * kWh_Wh; // 85 kWh
constexpr double BATTERY_NISSAN_LEAF = 30 * kWh_Wh; // 30 kWh
constexpr double BATTERY_PEUGEOT_ION = 16 * kWh_Wh; // 16 kWh
}

#endif
