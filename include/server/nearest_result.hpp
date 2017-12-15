#ifndef CHARGE_SERVER_NEAREST_RESULT_HPP
#define CHARGE_SERVER_NEAREST_RESULT_HPP

#include "common/coordinate.hpp"

namespace charge::server {
struct NearestResult {
    std::uint32_t id;
    common::Coordinate coordinate;
};
}

#endif
