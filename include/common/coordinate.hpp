#ifndef CHARGE_COMMON_COORDINATE_HPP
#define CHARGE_COMMON_COORDINATE_HPP

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <tuple>

namespace charge {
namespace common {

inline constexpr long double DEGREE_TO_RAD = 0.017453292519943295769236907684886;
inline constexpr long double RAD_TO_DEGREE = 1. / DEGREE_TO_RAD;

// Fixed point encoded coordinate to save memory
struct Coordinate {
    // devide by PRECISION to get floating point encoding
    static const constexpr double PRECISION = 1e6;
    std::int32_t lon;
    std::int32_t lat;

    static Coordinate from_floating(double lon, double lat) {
        return Coordinate{static_cast<std::int32_t>(lon * PRECISION),
                          static_cast<std::int32_t>(lat * PRECISION)};
    }

    std::tuple<double, double> to_floating() const {
        return std::make_tuple(lon / PRECISION, lat / PRECISION);
    }

    bool operator==(const Coordinate &other) const {
        return std::tie(lon, lat) == std::tie(other.lon, other.lat);
    }
};

inline std::ostream &operator<<(std::ostream &out, const Coordinate &coord) {
    out << "{" << (coord.lon / Coordinate::PRECISION) << "," << (coord.lat / Coordinate::PRECISION)
        << "}";
    return out;
}

inline long euclid_squared_distance(const Coordinate &lhs, const Coordinate &rhs) {
    const long dlat = lhs.lat - rhs.lat;
    const long dlon = lhs.lon - rhs.lon;

    return dlat * dlat + dlon * dlon;
}

inline double bearing(const Coordinate &lhs, const Coordinate &rhs) {
    auto[lhs_lon, lhs_lat] = lhs.to_floating();
    auto[rhs_lon, rhs_lat] = rhs.to_floating();
    const auto lon_diff = rhs_lon - lhs_lon;
    const auto lon_delta = DEGREE_TO_RAD * lon_diff;
    const auto lat1 = DEGREE_TO_RAD*lhs_lat;
    const auto lat2 = DEGREE_TO_RAD*rhs_lat;
    const auto y = std::sin(lon_delta) * std::cos(lat2);
    const auto x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(lon_delta);
    double result = RAD_TO_DEGREE * std::atan2(y, x);

    while (result < 0.0) {
        result += 360.0;
    }
    while (result >= 360.0) {
        result -= 360.0;
    }

    return result;
}

inline double haversine_distance(const Coordinate lhs, const Coordinate rhs) {
    const auto[ln1, lt1] = lhs.to_floating();
    const auto[ln2, lt2] = rhs.to_floating();

    const constexpr long double EARTH_RADIUS = 6372797.560856;

    const double dlat1 = lt1 * DEGREE_TO_RAD;
    const double dlong1 = ln1 * DEGREE_TO_RAD;
    const double dlat2 = lt2 * DEGREE_TO_RAD;
    const double dlong2 = ln2 * DEGREE_TO_RAD;

    const double dlong = dlong1 - dlong2;
    const double dlat = dlat1 - dlat2;

    const double aharv = std::pow(std::sin(dlat / 2.0), 2.0) +
                         std::cos(dlat1) * std::cos(dlat2) * std::pow(std::sin(dlong / 2.), 2);
    const double charv = 2. * std::atan2(std::sqrt(aharv), std::sqrt(1.0 - aharv));
    return EARTH_RADIUS * charv;
}

static const constexpr Coordinate INVALID_COORD = {std::numeric_limits<std::int32_t>::max(),
                                                   std::numeric_limits<std::int32_t>::max()};
} // namespace common
} // namespace charge

#endif
