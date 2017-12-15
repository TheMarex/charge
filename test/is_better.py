#!/usr/bin/env python3

import sys
import json

lhs = sys.argv[1]
rhs = sys.argv[2]

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

lhs_routes = index_routes(parse_results(lhs))
rhs_routes = index_routes(parse_results(rhs))

relative_error = []
absolute_error = []

better = 0
for key in lhs_routes:
    lhs_route = lhs_routes[key]
    if key not in rhs_routes:
        continue

    rhs_route = rhs_routes[key]
    lhs_duration = lhs_route["durations"][-1]
    rhs_duration = rhs_route["durations"][-1]
    lhs_consumption = lhs_route["consumptions"][-1]
    rhs_consumption = rhs_route["consumptions"][-1]
    relative_error.append(rhs_duration/lhs_duration - 1)
    absolute_error.append(rhs_duration-lhs_duration)
    if lhs_duration > rhs_duration:
        print("{}: not faster {} > {}".format(key, lhs_duration, rhs_duration))
    elif lhs_duration == rhs_duration:
        if lhs_consumption > rhs_consumption:
            print("{}: not efficient {} > {}".format(key, lhs_consumption, rhs_consumption))
    else:
        better += 1

not_found = 0
for key in rhs_routes:
    if key not in lhs_routes:
        print("not found {}".format(key))
        not_found += 1

avg_relative_error = sum(relative_error)/len(relative_error)
avg_absolute_error = sum(absolute_error)/len(absolute_error)
max_absolute_error = max(absolute_error)
max_relative_error = max(relative_error)
print("relative error: avg. {}%  max. {}% | absolute error: avg. {}s max. {}s | not found {}".format(avg_relative_error*100, max_relative_error*100, avg_absolute_error, max_absolute_error, not_found))
