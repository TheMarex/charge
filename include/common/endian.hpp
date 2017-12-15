#ifndef CHARGE_COMMON_ENDIAN_HPP
#define CHARGE_COMMON_ENDIAN_HPP

#include <cstdint>

namespace charge::common
{
    namespace detail
    {
        inline std::int16_t swap_bytes(std::uint16_t raw_bytes)
        {
            std::uint16_t raw_bytes_swapped = ((raw_bytes & 0xFF) << 8) | ((raw_bytes & 0xFF00) >> 8);
            return raw_bytes_swapped;
        }
    }

    inline std::int16_t little_to_big_endian(std::int16_t little_endian)
    {
        std::uint16_t raw_bytes = static_cast<std::uint16_t>(little_endian);
        std::uint16_t raw_bytes_swapped = detail::swap_bytes(raw_bytes);
        return static_cast<std::int16_t>(raw_bytes_swapped);
    }

    inline std::int16_t big_to_little_endian(std::int16_t big_endian)
    {
        std::uint16_t raw_bytes = static_cast<std::uint16_t>(big_endian);
        std::uint16_t raw_bytes_swapped = detail::swap_bytes(raw_bytes);
        return static_cast<std::int16_t>(raw_bytes_swapped);
    }
}

#endif
