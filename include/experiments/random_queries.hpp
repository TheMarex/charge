#ifndef CHARGE_COMMON_RANDOM_QUERIES_HPP
#define CHARGE_COMMON_RANDOM_QUERIES_HPP

#include "experiments/query.hpp"

#include "common/constants.hpp"
#include "common/irange.hpp"

#include <algorithm>
#include <cstdint>
#include <random>

namespace charge::experiments {

inline auto make_random_queries(const std::size_t seed, const std::size_t num_nodes,
                                const std::size_t num_queries) {
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<std::int32_t> distribution(0, num_nodes - 1);

    std::vector<Query> queries(num_queries);
    auto id_range = common::irange<std::uint32_t>(0, num_queries);
    std::transform(id_range.begin(), id_range.end(), queries.begin(), [&](const auto id) {
        return Query{distribution(generator), distribution(generator),
                     std::numeric_limits<double>::infinity(),
                     std::numeric_limits<double>::infinity(), id};
    });

    return queries;
}
}

#endif
