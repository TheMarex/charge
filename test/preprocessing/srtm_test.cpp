#include "preprocessing/srtm.hpp"

#include "common/endian.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::preprocessing;

TEST_CASE("Parse simple tiles", "[srtm]")
{
    {
        common::BinaryWriter writer{TEST_DIR "/N01E005.hgt"};

        std::vector<std::int16_t> tile {
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11,
            12, 13, 14, 15
        };
        std::transform(tile.begin(), tile.end(), tile.begin(), common::little_to_big_endian);
        writer.write(*tile.data(), tile.size());
    }

    {
        common::BinaryWriter writer{TEST_DIR "/S10W111.hgt"};

        std::vector<std::int16_t> tile {
            0, 1, 2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11,
            12, 13, 14, 15
        };
        std::transform(tile.begin(), tile.end(), tile.begin(), common::little_to_big_endian);
        writer.write(*tile.data(), tile.size());
    }

    auto elevation_data = read_srtm(TEST_DIR, common::Coordinate::from_floating(-180,-20), common::Coordinate::from_floating(10, 10));

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5, 10)) == 0);

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5, 2)) == Approx(0));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5, 1)) == Approx(12));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(6, 2)) == Approx(3));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(6, 1)) == Approx(15));

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5.5, 1.5)) == Approx(7.5));

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5, 1.5)) == Approx(6));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(6, 1.5)) == Approx(9));

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5.5, 1)) == Approx(13.5));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(5.5, 2)) == Approx(1.5));

    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(-111, -9)) == Approx(0));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(-111, -10)) == Approx(12));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(-110, -9)) == Approx(3));
    CHECK(elevation_data.interpolate(common::Coordinate::from_floating(-110, -10)) == Approx(15));
}
