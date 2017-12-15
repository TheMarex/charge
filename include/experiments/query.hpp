#ifndef CHARGE_EXPERIMENTS_QUERY_HPP
#define CHARGE_EXPERIMENTS_QUERY_HPP

#include <cstdint>

namespace charge::experiments {

struct Query {
    std::int32_t start;
    std::int32_t target;
    double min_consumption;
    double max_consumption;
    std::uint32_t id;
    std::uint32_t rank;
};

}

#endif
