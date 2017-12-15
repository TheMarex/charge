#ifndef CHARGER_COMMON_SERILIAZATION_HPP
#define CHARGER_COMMON_SERILIAZATION_HPP

#include "common/binary.hpp"
#include "common/csv.hpp"

#include <vector>

namespace charge
{
namespace common
{
namespace serialization
{
    template<typename T>
    void read(BinaryReader &reader, std::vector<T> &vector)
    {
        std::uint64_t count;
        reader.read(count, 1);
        vector.resize(count);
        reader.read(*vector.data(), count);
    }

    template<typename T>
    void write(BinaryWriter &writer, const std::vector<T> &vector)
    {
        writer.write(vector.size(), 1);
        writer.write(*vector.data(), vector.size());
    }

    template<class ... Types>
    void read(CSVReader<Types...> &reader, std::vector<std::tuple<Types...>> &vector)
    {
        typename CSVReader<Types...>::output_t t;
        while (reader.read(t))
            vector.push_back(t);
    }

    template<class ... Types>
    void write(CSVWriter<Types...> &writer, const std::vector<std::tuple<Types...>> &vector)
    {
        for (const auto& t : vector)
            writer.write(t);
    }
}
}
}

#endif
