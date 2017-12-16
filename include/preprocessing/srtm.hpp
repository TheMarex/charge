#ifndef CHARGE_PREPROCESSING_SRTM_HPP
#define CHARGE_PREPROCESSING_SRTM_HPP

#include "common/binary.hpp"
#include "common/coordinate.hpp"
#include "common/irange.hpp"
#include "common/endian.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <atomic>

#include <sys/stat.h>

namespace charge::preprocessing {

namespace detail {
// Formats to N00E000, S00W000
inline std::string format_coordinate(int lon, int lat) {
    std::string coordinate;

    if (lat > 0) {
        coordinate += "N";
        if (lat >= 10) {
            coordinate += std::to_string(lat);
        } else {
            coordinate += "0" + std::to_string(lat);
        }
    } else {
        coordinate += "S";
        if (lat <= -10) {
            coordinate += std::to_string(-lat);
        } else {
            coordinate += "0" + std::to_string(-lat);
        }
    }

    if (lon > 0) {
        coordinate += "E";
        if (lon >= 100) {
            coordinate += std::to_string(lon);
        } else if (lon >= 10) {
            coordinate += "0" + std::to_string(lon);
        } else {
            coordinate += "00" + std::to_string(lon);
        }
    } else {
        coordinate += "W";
        if (lon <= -100) {
            coordinate += std::to_string(-lon);
        } else if (lon <= -10) {
            coordinate += "0" + std::to_string(-lon);
        } else {
            coordinate += "00" + std::to_string(-lon);
        }
    }

    return coordinate;
}
struct Tile {
    double interpolate(common::Coordinate coordinate) const {
        auto[lon, lat] = coordinate.to_floating();
        assert(lon <= max_lon);
        assert(lon >= min_lon);
        assert(lat <= max_lat);
        assert(lat >= min_lat);

        auto u = (lon - min_lon) / (max_lon - min_lon);
        auto v = 1 - (lat - min_lat) / (max_lat - min_lat);

        return interpolate(u, v);
    }

    double interpolate(double u, double v) const {
        // texture contains n values for n-1 pixels
        // where each value is the upper-left corner of
        // a pixel
        auto num_horizontal_cells = columns - 1;
        auto num_vertical_cells = rows - 1;
        double column = u * num_horizontal_cells;
        double row = v * num_vertical_cells;

        std::uint32_t row0 = row;
        std::uint32_t column0 = column;
        row0 = std::min(row0, rows - 2);
        column0 = std::min(column0, columns - 2);
        std::uint32_t row1 = row0 + 1;
        std::uint32_t column1 = column0 + 1;
        assert(row0 < rows);
        assert(column0 < columns);
        assert(row1 < rows);
        assert(column1 < columns);

        // row-major
        auto h_00 = heights[column0 + row0 * columns];
        auto h_01 = heights[column0 + row1 * columns];
        auto h_10 = heights[column1 + row0 * columns];
        auto h_11 = heights[column1 + row1 * columns];

        auto h = h_00 * (1 - u) * (1 - v) + h_10 * u * (1 - v) + h_01 * (1 - u) * v + h_11 * u * v;

        return h;
    }

    bool contains(common::Coordinate coordinate) const {
        auto[lon, lat] = coordinate.to_floating();

        return min_lon <= lon && max_lon >= lon && min_lat <= lat && max_lat >= lat;
    }

    double min_lon;
    double min_lat;
    double max_lon;
    double max_lat;
    std::uint32_t rows;
    std::uint32_t columns;
    std::vector<std::int16_t> heights;
};
}

struct ElevationTiles {
    double interpolate(common::Coordinate coordinate) const {
        auto iter = std::find_if(tiles.begin(), tiles.end(),
                                 [=](const auto &tile) { return tile.contains(coordinate); });

        if (iter == tiles.end())
        {
            no_data_count++;
            return default_value;
        }
        else
        {
            data_count++;
            return iter->interpolate(coordinate);
        }
    }

    double default_value;
    std::vector<detail::Tile> tiles;

    mutable std::uint32_t no_data_count = 0;
    mutable std::uint32_t data_count = 0;

};

inline auto read_srtm(const std::string &base_path, common::Coordinate sw, common::Coordinate ne) {
    std::int32_t min_lon = std::floor(sw.lon / common::Coordinate::PRECISION);
    std::int32_t min_lat = std::floor(sw.lat / common::Coordinate::PRECISION);
    std::int32_t max_lon = std::ceil(ne.lon / common::Coordinate::PRECISION);
    std::int32_t max_lat = std::ceil(ne.lat / common::Coordinate::PRECISION);

    std::vector<detail::Tile> tiles;

    for (auto lon : common::irange<std::int32_t>(min_lon, max_lon)) {
        for (auto lat : common::irange<std::int32_t>(min_lat, max_lat)) {
            std::string filename = base_path + "/" + detail::format_coordinate(lon, lat) + ".hgt";

            struct stat buffer;
            if (stat(filename.c_str(), &buffer) != 0) {
                continue;
            }

            common::BinaryReader reader{filename};
            auto size = reader.size() / sizeof(std::int16_t);
            std::size_t num_rows = std::sqrt(size);
            if (size != num_rows * num_rows) {
                throw std::runtime_error(filename + " is not quadratic: " + std::to_string(size));
            }

            detail::Tile tile;
            tile.min_lon = lon;
            tile.max_lon = lon + 1;
            tile.min_lat = lat;
            tile.max_lat = lat + 1;
            tile.heights.resize(size);
            tile.rows = num_rows;
            tile.columns = num_rows;
            reader.read(*tile.heights.data(), size);
            std::transform(tile.heights.begin(), tile.heights.end(), tile.heights.begin(), common::big_to_little_endian);

            tiles.push_back(std::move(tile));
        }
    }

    return ElevationTiles{0, std::move(tiles)};
}
}

#endif
