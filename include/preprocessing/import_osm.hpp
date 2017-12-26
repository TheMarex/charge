#ifndef CHARGE_COMMON_IMPORT_OSM_HPP
#define CHARGE_COMMON_IMPORT_OSM_HPP

#include "preprocessing/srtm.hpp"

#include "common/coordinate.hpp"
#include "common/dyn_graph.hpp"
#include "common/graph_transform.hpp"
#include "common/irange.hpp"
#include "common/weighted_graph.hpp"

#include "ev/consumption_model.hpp"
#include "ev/graph.hpp"

#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/tags/tags_filter.hpp>
#include <osmium/visitor.hpp>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

namespace charge {
namespace preprocessing {

struct OSMNetwork {
    struct NodeMetaData {
        std::int32_t height;
        common::Coordinate coordinate;

        bool operator==(const NodeMetaData &other) const {
            return std::tie(other.height, other.coordinate) == std::tie(height, coordinate);
        }
    };
    struct EdgeMetaData {
        double min_speed;
        double max_speed;
        bool is_tunnel;

        bool operator==(const EdgeMetaData &other) const {
            return std::tie(other.min_speed, other.max_speed, other.is_tunnel) == std::tie(min_speed, max_speed, is_tunnel);
        }
        bool operator<(const EdgeMetaData &other) const {
            return std::tie(other.min_speed, other.max_speed) < std::tie(min_speed, max_speed);
        }
    };
    using Edge = std::tuple<std::uint32_t, std::uint32_t, EdgeMetaData>;

    std::vector<Edge> edges;
    std::vector<NodeMetaData> nodes;
};

namespace detail {

class ParsingHandler : public osmium::handler::Handler {
  public:
    ParsingHandler(OSMNetwork &network)
        : network(network), highway_filter{false}, access_filter{true}, oneway_filter{false},
          reversed_filter{false}, tunnel_filter{false} {
        // allow all highways by default but blacklist certain highways
        highway_filter.add_rule(false, osmium::TagMatcher{"highway", "pedestrian"});
        highway_filter.add_rule(false, osmium::TagMatcher{"highway", "cycleway"});
        highway_filter.add_rule(false, osmium::TagMatcher{"highway", "footway"});
        highway_filter.add_rule(true, osmium::TagMatcher{"highway"});

        // if there is an access tag we don't whitelist we don't allow the way
        access_filter.add_rule(true, osmium::TagMatcher{"access", "yes"});
        access_filter.add_rule(true, osmium::TagMatcher{"vehicle", "yes"});
        access_filter.add_rule(true, osmium::TagMatcher{"motor_vehicle", "yes"});
        access_filter.add_rule(false, osmium::TagMatcher{"access"});
        access_filter.add_rule(false, osmium::TagMatcher{"vehicle"});
        access_filter.add_rule(false, osmium::TagMatcher{"motor_vehicle"});

        oneway_filter.add_rule(false, osmium::TagMatcher{"oneway", "no"});
        oneway_filter.add_rule(true, osmium::TagMatcher{"oneway", "yes"});
        oneway_filter.add_rule(true, osmium::TagMatcher{"highway", "motorway"});
        oneway_filter.add_rule(true, osmium::TagMatcher{"highway", "motorway_link"});

        tunnel_filter.add_rule(true, osmium::TagMatcher{"tunnel", "yes"});
        tunnel_filter.add_rule(true, osmium::TagMatcher{"avalanche_protector", "yes"});
        tunnel_filter.add_rule(true, osmium::TagMatcher{"covered", "yes"});

        reversed_filter.add_rule(true, osmium::TagMatcher{"oneway", "-1"});

        default_max_speeds["motorway"] = "140";
        default_max_speeds["motorway_link"] = "60";
        default_max_speeds["trunk"] = "100";
        default_max_speeds["trunk_link"] = "60";
        default_max_speeds["primary"] = "100";
        default_max_speeds["primary_link"] = "50";
        default_max_speeds["secondary"] = "60";
        default_max_speeds["secondary_link"] = "50";
        default_max_speeds["tertiary"] = "60";
        default_max_speeds["tertiary_link"] = "50";
        default_max_speeds["residential"] = "50";
        default_max_speeds["unclassified"] = "50";
        default_max_speeds["living_street"] = "15";

        default_min_speeds["motorway"] = "80";
        default_min_speeds["motorway_link"] = "30";
        default_min_speeds["trunk"] = "80";
        default_min_speeds["trunk_link"] = "30";
        default_min_speeds["primary"] = "50";
        default_min_speeds["primary_link"] = "30";
        default_min_speeds["secondary"] = "50";
        default_min_speeds["secondary_link"] = "30";
        default_min_speeds["tertiary"] = "50";
        default_min_speeds["tertiary_link"] = "30";
        default_min_speeds["residential"] = "30";
        default_min_speeds["unclassified"] = "30";
        default_min_speeds["living_street"] = "10";
    }

    void node(const osmium::Node &) noexcept {}
    void relation(const osmium::Relation &) noexcept {}

    void way(const osmium::Way &way) noexcept {
        if (way.nodes().size() < 2)
            return;

        bool is_oneway = false;
        bool is_highway = false;
        bool is_accessible = true;
        bool is_reverse = false;
        bool is_tunnel = false;
        for (const auto &tag : way.tags()) {
            is_oneway |= oneway_filter(tag);
            is_reverse |= reversed_filter(tag);
            is_highway |= highway_filter(tag);
            is_accessible &= access_filter(tag);
            is_tunnel |= tunnel_filter(tag);
        }

        if (!is_highway || !is_accessible) {
            return;
        }
        assert(is_highway);
        assert(is_accessible);

        auto[forward_max_speed, backward_max_speed] = compute_max_speed(way);
        auto[forward_min_speed, backward_min_speed] = compute_min_speed(way);

        for (const auto &node : way.nodes()) {
            if (auto iter = osm_to_local.find(node.ref()); iter == osm_to_local.end()) {
                osm_to_local[node.ref()] = network.nodes.size();
                auto c = common::Coordinate::from_floating(node.lon(), node.lat());
                network.nodes.push_back(OSMNetwork::NodeMetaData{0, c});
            }
        }

        if (is_reverse) {
            auto rbegin = std::make_reverse_iterator(way.nodes().end());
            auto rend = std::make_reverse_iterator(way.nodes().begin());
            for (auto current = rbegin, next = std::next(current); next != rend;
                 ++current, ++next) {
                auto start = osm_to_local[current->ref()];
                auto target = osm_to_local[next->ref()];

                network.edges.push_back({start, target, {forward_min_speed, forward_max_speed, is_tunnel}});
            }
        } else if (is_oneway) {
            auto begin = way.nodes().begin();
            auto end = way.nodes().end();
            for (auto current = begin, next = std::next(current); next != end; ++current, ++next) {
                auto start = osm_to_local[current->ref()];
                auto target = osm_to_local[next->ref()];

                network.edges.push_back({start, target, {forward_min_speed, forward_max_speed, is_tunnel}});
            }
        } else {
            auto begin = way.nodes().begin();
            auto end = way.nodes().end();
            for (auto current = begin, next = std::next(current); next != end; ++current, ++next) {
                auto start = osm_to_local[current->ref()];
                auto target = osm_to_local[next->ref()];

                network.edges.push_back({start, target, {forward_min_speed, forward_max_speed, is_tunnel}});
                network.edges.push_back({target, start, {backward_min_speed, backward_max_speed, is_tunnel}});
            }
        }
    }

  private:
    const char *default_max_speed(const osmium::Way &way) const {
        const char *highway = way.get_value_by_key("highway");
        if (auto iter = default_max_speeds.find(highway); iter != default_max_speeds.end()) {
            return iter->second;
        }

        return "5";
    }

    const char *default_min_speed(const osmium::Way &way) const {
        const char *highway = way.get_value_by_key("highway");
        if (auto iter = default_min_speeds.find(highway); iter != default_min_speeds.end()) {
            return iter->second;
        }

        const char *motorroad = way.get_value_by_key("motorroad", "no");
        if (std::string("yes") == motorroad) {
            return "60";
        }

        return "5";
    }

    double value_to_number(const std::string &value, const char *fallback_value) const {
        auto begin_mph = value.find("mph");

        try {
            std::size_t number_chars = 0;
            auto number = std::stof(value, &number_chars);
            if (number_chars > 0) {
                if (begin_mph == std::string::npos) {
                    return number;
                } else {
                    return number * 1.60934;
                }
            }
        } catch (std::invalid_argument) {
            return std::stof(fallback_value);
        }

        return std::stof(fallback_value);
    }

    std::tuple<double, double> compute_max_speed(const osmium::Way &way) const noexcept {
        auto default_max_speed_value = default_max_speed(way);
        auto max_speed_value = way.get_value_by_key("maxspeed", default_max_speed_value);
        auto forward_max_speed_value = way.get_value_by_key("maxspeed:forward", max_speed_value);
        auto backward_max_speed_value = way.get_value_by_key("maxspeed:backward", max_speed_value);
        auto forward_max_speed = std::max(5.0, std::min(140.0, value_to_number(forward_max_speed_value, default_max_speed_value)));
        auto backward_max_speed = std::max(5.0, std::min(140.0, value_to_number(backward_max_speed_value, default_max_speed_value)));
        return std::make_tuple(forward_max_speed, backward_max_speed);
    }

    std::tuple<double, double> compute_min_speed(const osmium::Way &way) const noexcept {
        auto default_min_speed_value = default_min_speed(way);
        auto min_speed_value = way.get_value_by_key("minspeed", default_min_speed_value);
        auto forward_min_speed_value = way.get_value_by_key("minspeed:forward", min_speed_value);
        auto backward_min_speed_value = way.get_value_by_key("minspeed:backward", min_speed_value);
        auto forward_min_speed = std::max(5.0, std::min(140.0, value_to_number(forward_min_speed_value, default_min_speed_value)));
        auto backward_min_speed = std::max(5.0, std::min(140.0, value_to_number(backward_min_speed_value, default_min_speed_value)));
        return std::make_tuple(forward_min_speed, backward_min_speed);
    }

    OSMNetwork &network;

    osmium::TagsFilter highway_filter;
    osmium::TagsFilter access_filter;
    osmium::TagsFilter oneway_filter;
    osmium::TagsFilter reversed_filter;
    osmium::TagsFilter tunnel_filter;

    std::unordered_map<const char *, const char *> default_max_speeds;
    std::unordered_map<const char *, const char *> default_min_speeds;

    std::unordered_map<osmium::unsigned_object_id_type, std::uint32_t> osm_to_local;
};
}

inline OSMNetwork read_network(const std::string &base_path) {
    OSMNetwork network;

    osmium::io::File file{base_path + ".osm.pbf"};
    osmium::io::Reader reader{file};

    osmium::index::map::SparseMemArray<osmium::unsigned_object_id_type, osmium::Location>
        location_index;
    osmium::handler::NodeLocationsForWays<decltype(location_index)> location_handler{
        location_index};

    detail::ParsingHandler parsing_handler{network};

    osmium::apply(reader, location_handler, parsing_handler);

    return network;
}

inline auto annotate_elevation(const std::string &base_path, OSMNetwork &network) {
    common::Coordinate sw;
    common::Coordinate ne;

    for (const auto &data : network.nodes) {
        sw.lat = std::min(data.coordinate.lat, sw.lat);
        sw.lon = std::min(data.coordinate.lon, sw.lon);
        ne.lat = std::max(data.coordinate.lat, ne.lat);
        ne.lon = std::max(data.coordinate.lon, ne.lon);
    }

    auto elevation_data = read_srtm(base_path, sw, ne);

    for (auto &data : network.nodes)
        data.height = elevation_data.interpolate(data.coordinate);

    return std::make_tuple(elevation_data.data_count, elevation_data.no_data_count);
}

// Removes excessive degree 2 nodes where possible
inline auto simplify_network(OSMNetwork network, std::size_t small_component_size = 1000) {
    auto num_nodes = network.nodes.size();
    std::vector<bool> removed_node(num_nodes, false);

    using edge_t = common::DynDataGraph<OSMNetwork::EdgeMetaData>::edge_t;
    std::vector<edge_t> edges(network.edges.size());
    std::transform(network.edges.begin(), network.edges.end(), edges.begin(), [](const auto &edge) {
        auto[start, target, data] = edge;
        return edge_t{start, target, data};
    });
    std::sort(edges.begin(), edges.end());

    common::DynDataGraph<OSMNetwork::EdgeMetaData> graph{num_nodes, edges};

    std::vector<std::size_t> degree(num_nodes, 0);
    std::vector<std::size_t> component(num_nodes, common::INVALID_ID);
    std::vector<std::size_t> component_sizes;

    {
        auto undirected_graph = common::toUndirected(graph);
        degree = computeDegree(undirected_graph);
        std::tie(component, component_sizes) = computeComponents(undirected_graph);
    }

    auto filtered_edges = graph.edges();
    auto new_end = std::remove_if(filtered_edges.begin(), filtered_edges.end(), [&](const auto &edge) {
        return component_sizes[component[edge.start]] < small_component_size ||
               component_sizes[component[edge.target]] < small_component_size;
    });
    auto new_size = new_end - filtered_edges.begin();
    filtered_edges.resize(new_size);
    graph = {num_nodes, filtered_edges};

    // Contract degree 2 nodes (note: contraction can only degrease the degree in this case)
    std::size_t removed;
    do {
        removed = 0;
        for (auto from : common::irange<std::uint32_t>(0, num_nodes)) {
            for (const auto from_edge : graph.edges(from)) {
                auto via = graph.target(from_edge);
                if (degree[via] != 2)
                    continue;

                bool contracted = false;
                for (const auto to_edge : graph.edges(via)) {
                    auto to = graph.target(to_edge);
                    if (to == from)
                        continue;

                    auto downhill = (network.nodes[from].height >= network.nodes[via].height &&
                                     network.nodes[via].height >= network.nodes[to].height);
                    auto uphill = (network.nodes[from].height <= network.nodes[via].height &&
                                   network.nodes[via].height <= network.nodes[to].height);
                    bool forward_compatible = graph.weight(from_edge) == graph.weight(to_edge);
                    bool forward_is_tunnel = graph.weight(from_edge).is_tunnel && graph.weight(to_edge).is_tunnel;

                    if (forward_is_tunnel || (forward_compatible && (downhill || uphill))) {
                        auto backward_from_edge = graph.edge(via, from);
                        auto backward_to_edge = graph.edge(to, via);

                        // oneway street
                        if (backward_from_edge == common::INVALID_ID &&
                            backward_to_edge == common::INVALID_ID) {

                            auto weight = graph.weight(from_edge);

                            graph.remove(from, from_edge);
                            graph.remove(via, to_edge);
                            graph.insert({from, to, weight});
                            contracted = true;
                        }
                        // two way street
                        else if (backward_from_edge != common::INVALID_ID &&
                                 backward_to_edge != common::INVALID_ID) {
                            bool backward_compatible =
                                graph.weight(backward_from_edge) == graph.weight(backward_to_edge);
                            bool backward_is_tunnel = graph.weight(backward_from_edge).is_tunnel && graph.weight(backward_to_edge).is_tunnel;

                            if ((forward_is_tunnel && backward_is_tunnel) || backward_compatible) {
                                auto forward_weight = graph.weight(from_edge);
                                auto backward_weight = graph.weight(backward_from_edge);

                                graph.remove(from, from_edge);
                                graph.remove(via, to_edge);
                                graph.remove(via, backward_from_edge);
                                graph.remove(to, backward_to_edge);
                                graph.insert({from, to, forward_weight});
                                graph.insert({to, from, backward_weight});
                                contracted = true;
                            }
                        }
                    }

                    if (contracted)
                        break;
                }

                if (contracted) {
                    removed++;
                    // we can't process any more edges going out `from` since we just invalidated
                    // the edge
                    break;
                }
            }
        }
    } while (removed > 0);

    network.edges.clear();

    std::vector<bool> used(graph.num_nodes(), false);
    for (const auto &edge : graph.edges()) {
        network.edges.push_back(OSMNetwork::Edge{edge.start, edge.target, edge.weight});
        used[edge.start] = true;
        used[edge.target] = true;
    }

    std::vector<std::uint32_t> old_to_new(graph.num_nodes(), common::INVALID_ID);
    std::uint32_t new_id = 0;
    for (auto node : common::irange<std::uint32_t>(0, graph.num_nodes())) {
        if (used[node]) {
            old_to_new[node] = new_id++;
            assert(node >= old_to_new[node]);
            network.nodes[old_to_new[node]] = network.nodes[node];
        }
    }
    network.nodes.resize(new_id);

    for (auto &edge : network.edges) {
        auto & [ start, target, weight ] = edge;
        start = old_to_new[start];
        target = old_to_new[target];
    }

    return network;
}

auto coordinates_from_network(const OSMNetwork &network) {
    std::vector<common::Coordinate> coordinates(network.nodes.size());
    std::transform(network.nodes.begin(), network.nodes.end(), coordinates.begin(),
                   [](const auto &data) { return data.coordinate; });
    return coordinates;
}

auto heights_from_network(const OSMNetwork &network) {
    std::vector<std::int32_t> heights(network.nodes.size());
    std::transform(network.nodes.begin(), network.nodes.end(), heights.begin(),
                   [](const auto &data) { return data.height; });
    return heights;
}

auto tradeoff_graph_from_network(const OSMNetwork &network, const ev::ConsumptionModel &model) {
    using GraphT = common::WeightedGraph<ev::LimitedTradeoffFunction>;
    using edge_t = typename GraphT::edge_t;
    using node_id_t = typename GraphT::node_id_t;

    std::vector<edge_t> edges;
    for (const auto &edge : network.edges) {
        const auto & [ start, target, data ] = edge;

        auto length = common::haversine_distance(network.nodes[start].coordinate,
                                                 network.nodes[target].coordinate);
        auto slope =
            length > 0 ? (network.nodes[target].height - network.nodes[start].height) / length : 0;
        auto min_speed = data.min_speed;
        auto max_speed = data.max_speed;
        if (min_speed > max_speed)
            min_speed = max_speed;
        assert(min_speed <= max_speed);
        auto weight = model.tradeoff_function(length, slope, min_speed, max_speed);

        edges.push_back(edge_t{static_cast<typename GraphT::node_id_t>(start),
                               static_cast<typename GraphT::node_id_t>(target), weight});
    }

    std::sort(edges.begin(), edges.end());

    return ev::TradeoffGraph{GraphT{network.nodes.size(), edges}};
}
}
}

#endif
