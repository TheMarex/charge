#!/usr/bin/env python3

import sys
import json

if len(sys.argv) < 3:
    print("{} show routes.json from;to".format(sys.argv[0]))
    print("{} diff routes.json other_route.json".format(sys.argv[0]))
    sys.exit(1)
else:
    mode = sys.argv[1]

def index_routes(results):
    query_to_route = {}
    for result in results:
        key = "{};{}".format(result["start"], result["target"])
        query_to_route[key] = result["routes"][0]
    return query_to_route


def parse_results(path):
    with open(path) as f:
        lines = f.readlines()
        results = [json.loads(l) for l in lines]
        results = [r for r in results if r != None]
        return results

def make_feature(geometry, consumption, duration, good):
    return {
            "type": "Feature",
            "properties": {
                "consumption": consumption,
                "duration": duration,
                "stroke": good and "#00ff00" or "#ff0000",
                "stroke-width": 2,
                "stroke-opacity": 0.5
                },
            "geometry": {
                "type": "LineString",
                "coordinates": geometry,
                }
            }

def make_stop(geometry, consumption, duration, id, good):
    return {
            "type": "Feature",
            "properties": {
                "id": id,
                "duration": duration,
                "consumption": consumption,
                "marker-color": good and "#00ff00" or "#ff0000",
                "marker-size": "medium",
                "marker-symbol": "star"
                },
            "geometry": {
                "type": "Point",
                "coordinates": geometry,
                }
            }

def charging_stops(route, good):
    path = route["path"]
    consumptions = route["consumptions"]
    durations = route["durations"]
    geometry = route["geometry"]

    stops = []

    for idx in range(len(path)-1):
        if path[idx] == path[idx+1]:
            duration = durations[idx+1]-durations[idx]
            consumption = consumptions[idx+1]-consumptions[idx]
            stops.append(make_stop(geometry[idx], consumption, duration, path[idx], good))

    return stops

def make_visualization(route):
    path = route["path"]
    geometry = route["geometry"]
    consumption = route["consumptions"]
    duration = route["durations"]

    feature = make_feature(geometry, consumption, duration, False)
    return {
             "type": "FeatureCollection",
             "features": [feature] + charging_stops(route, False)
            }

def make_diff(lhs_route, rhs_route):
    lhs_path = lhs_route["path"]
    lhs_geometry = lhs_route["geometry"]
    lhs_consumption = lhs_route["consumptions"]
    lhs_duration = lhs_route["durations"]
    rhs_path = rhs_route["path"]
    rhs_geometry = rhs_route["geometry"]
    rhs_consumption = rhs_route["consumptions"]
    rhs_duration = rhs_route["durations"]

    min_len = min(len(lhs_path), len(rhs_path))

    first_diff = 0
    for idx in range(min_len):
        if lhs_path[idx] != rhs_path[idx]:
            first_diff = idx
            break

    last_diff = 0
    for idx in range(2, min_len + 1):
        if lhs_path[-idx] != rhs_path[-idx] or len(lhs_path) - idx == first_diff or len(rhs_path) - idx == first_diff:
            last_diff = -idx
            break

    assert(first_diff > 0)

    query = "{},{}".format(lhs_path[0], lhs_path[-1])
    sub_query = "{},{}".format(lhs_path[first_diff-1], lhs_path[last_diff+1])
    print("Query: {} - {}".format(query, sub_query))
    lhs_diff_geometry = lhs_geometry[first_diff-1:(last_diff+1)]
    rhs_diff_geometry = rhs_geometry[first_diff-1:(last_diff+1)]
    lhs_diff_consumption = lhs_consumption[last_diff+1] - lhs_consumption[first_diff-1]
    rhs_diff_consumption = rhs_consumption[last_diff+1] - rhs_consumption[first_diff-1]
    lhs_diff_duration = lhs_duration[last_diff+1] - lhs_duration[first_diff-1]
    rhs_diff_duration = rhs_duration[last_diff+1] - rhs_duration[first_diff-1]

    lhs_feature = make_feature(lhs_diff_geometry, lhs_diff_consumption, lhs_diff_duration, False)
    rhs_feature = make_feature(rhs_diff_geometry, rhs_diff_consumption, rhs_diff_duration, True)
    return {
             "type": "FeatureCollection",
             "features": [lhs_feature, rhs_feature] + charging_stops(lhs_route, False) + charging_stops(rhs_route, True)
            }


if mode == "diff":
    lhs = sys.argv[2]
    rhs = sys.argv[3]

    lhs_routes = index_routes(parse_results(lhs))
    rhs_routes = index_routes(parse_results(rhs))

    for key in lhs_routes:
        lhs_route = lhs_routes[key]
        if key not in rhs_routes:
            continue

        rhs_route = rhs_routes[key]
        lhs_duration = lhs_route["durations"][-1]
        rhs_duration = rhs_route["durations"][-1]
        if lhs_duration > rhs_duration:
            print(json.dumps(make_diff(lhs_route, rhs_route)))
elif mode == "show":
    path = sys.argv[2]
    name = sys.argv[3]
    routes = index_routes(parse_results(path))
    print(json.dumps(make_visualization(routes[name])))




