#!/usr/bin/env python3

import urllib.request
import json
import sys
import re

rate_exp = re.compile("([\d\.]+)\s*k[Ww]")
url = "https://www.tesla.com/all-locations"

def make_feature(charger):
    chargers_string = charger["chargers"]
    best_kw = 0.0
    for match in rate_exp.finditer(chargers_string):
        kw = float(match.group(1))
        best_kw = max(kw, best_kw)

    return {
            "type": "Feature",
            "properties": {
                "rate": best_kw * 1000,
                },
            "geometry": {
                "type": "Point",
                "coordinates": [float(charger["longitude"]), float(charger["latitude"])]
                }

            }

def make_feature_collection(features):
    return {
            "type": "FeatureCollection",
            "features": features
            }

with urllib.request.urlopen(url) as req:
    chargers = json.load(req)

features = [make_feature(charger) for charger in chargers]
fc = make_feature_collection([f for f in features if f["properties"]["rate"] > 0])
print(json.dumps(fc))
