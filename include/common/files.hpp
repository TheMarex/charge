#ifndef CHARGE_COMMON_FILES_HPP
#define CHARGE_COMMON_FILES_HPP

#include "common/serialization.hpp"
#include "common/coordinate.hpp"
#include "common/weighted_graph.hpp"

namespace charge {
namespace common {
namespace files {

template <typename T>
WeightedGraph<T> read_weighted_graph(const std::string &base_path) {
    std::vector<typename WeightedGraph<T>::edge_id_t> first_out;
    std::vector<typename WeightedGraph<T>::node_id_t> head;
    std::vector<typename WeightedGraph<T>::weight_t> weight;

    {
        BinaryReader reader(base_path + "/first_out");
        serialization::read(reader, first_out);
    }
    {
        BinaryReader reader(base_path + "/head");
        serialization::read(reader, head);
    }
    {
        BinaryReader reader(base_path + "/weight");
        serialization::read(reader, weight);
    }

    return WeightedGraph<T>(std::move(first_out), std::move(head),
                            std::move(weight));
}

template <typename T>
void write_weighted_graph(const std::string &base_path,
                          const WeightedGraph<T> &graph) {
    auto [first_out, head, weight] = WeightedGraph<T>::unwrap(graph);

    {
        BinaryWriter writer(base_path + "/first_out");
        serialization::write(writer, first_out);
    }
    {
        BinaryWriter writer(base_path + "/head");
        serialization::write(writer, head);
    }
    {
        BinaryWriter writer(base_path + "/weight");
        serialization::write(writer, weight);
    }
}

inline auto read_coordinates(const std::string &base_path) {
    std::vector<Coordinate> coordinates;
    BinaryReader reader(base_path + "/coordinates");
    serialization::read(reader, coordinates);
    return coordinates;
}
inline void write_coordinates(const std::string &base_path, const std::vector<Coordinate> &coordinates) {
    BinaryWriter writer(base_path + "/coordinates");
    serialization::write(writer, coordinates);
}

inline auto read_heights(const std::string &base_path) {
    std::vector<std::int32_t> heights;
    BinaryReader reader(base_path + "/heights");
    serialization::read(reader, heights);
    return heights;
}
inline void write_heights(const std::string &base_path, const std::vector<std::int32_t> &heights) {
    BinaryWriter writer(base_path + "/heights");
    serialization::write(writer, heights);
}
}
}
}

#endif
