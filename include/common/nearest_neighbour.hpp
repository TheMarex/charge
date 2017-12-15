#ifndef CHARGE_COMMON_NEAREST_NEIGHBOUR_HPP
#define CHARGE_COMMON_NEAREST_NEIGHBOUR_HPP

#include "common/coordinate.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <vector>

namespace charge::common {

template <std::size_t MIN_LEAF_SIZE = 128> class NearestNeighbour {
  public:
    NearestNeighbour(const std::vector<Coordinate> &coordinates)
        : values(coordinates.size()), coordinates(coordinates) {
        std::iota(values.begin(), values.end(), 0);
        partition_x(values.begin(), values.end());
    }

    std::size_t nearest(const Coordinate &coordinate) const {
        std::size_t level = 0;

        auto begin = values.begin();
        auto end = values.end();
        auto min = begin;

        using iter = decltype(begin);
        std::vector<std::tuple<iter, iter>> queue;

        queue.push_back({begin, end});
        while (!queue.empty()) {
            auto[begin, end] = queue.back();
            queue.pop_back();
            auto size = std::distance(begin, end);
            if (size <= MIN_LEAF_SIZE) {
                while (begin != end) {
                    if (euclid_squared_distance(coordinate, coordinates[*min]) >
                        euclid_squared_distance(coordinate, coordinates[*begin]))
                        min = begin;
                    begin++;
                }

                continue;
            }
            assert(size > 2);
            const auto mid = begin + size / 2;
            assert(mid != begin);
            assert(mid != end);

            if (euclid_squared_distance(coordinate, coordinates[*min]) >
                euclid_squared_distance(coordinate, coordinates[*mid]))
                min = mid;

            if (level % 2 == 0) {
                if (coordinate.lon < coordinates[*mid].lon) {
                    queue.push_back({begin, mid});
                } else if (coordinate.lon > coordinates[*mid].lon) {
                    queue.push_back({std::next(mid), end});
                } else {
                    assert(coordinate.lon == coordinates[*mid].lon);
                    queue.push_back({begin, mid});
                    queue.push_back({std::next(mid), end});
                }
            } else {
                if (coordinate.lat < coordinates[*mid].lat) {
                    queue.push_back({begin, mid});
                } else if (coordinate.lat > coordinates[*mid].lat) {
                    queue.push_back({std::next(mid), end});
                } else {
                    assert(coordinate.lat == coordinates[*mid].lat);
                    queue.push_back({begin, mid});
                    queue.push_back({std::next(mid), end});
                }
            }
            level++;
        }

        assert(min != end);
        return *min;
    }

  private:
    template <typename Iter> void partition_x(Iter begin, Iter end) {
        auto size = std::distance(begin, end);
        if (size <= MIN_LEAF_SIZE) {
            return;
        }
        assert(size > 2);
        auto mid = begin + size / 2;
        assert(mid != begin);
        assert(mid != end);

        std::nth_element(begin, mid, end, [this](const auto lhs, const auto rhs) {
            return coordinates[lhs].lon < coordinates[rhs].lon;
        });

        partition_y(begin, mid);
        partition_y(std::next(mid), end);
    }

    template <typename Iter> void partition_y(Iter begin, Iter end) {
        auto size = std::distance(begin, end);
        if (size <= MIN_LEAF_SIZE) {
            return;
        }
        assert(size > 2);
        const auto mid = begin + size / 2;
        assert(mid != begin);
        assert(mid != end);

        std::nth_element(begin, mid, end, [this](const auto lhs, const auto rhs) {
            return coordinates[lhs].lat < coordinates[rhs].lat;
        });

        partition_x(begin, mid);
        partition_x(std::next(mid), end);
    }

    // recursively partitioned alternating by x and y
    std::vector<std::size_t> values;
    const std::vector<Coordinate> &coordinates;
};
}

#endif
