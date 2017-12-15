#/bin/env/python

import urllib.request
import os.path
import sys

fn = "N%02iE%03i.hgt"
url = "https://cloud.sdsc.edu/v1/AUTH_opentopography/Raster/SRTM_GL1/SRTM_GL1_srtm/North/North_30_60/%s"


if len(sys.argv) < 2:
    print("{} min_lon,min_lat,max_lon,max_lat".format(sys.argv[0]))
    sys.exit(1)

bb = [float(b) for b in sys.argv[1].split(",")]

min_lon = round(bb[0])
min_lat = round(bb[1])
max_lon = round(bb[2])
max_lat = round(bb[3])

print("Downloading {}x{} ({}) tiles...".format((max_lon - min_lon), (max_lat - min_lat), (max_lon - min_lon)*(max_lat - min_lat)))

for lat in range(min_lat, max_lat):
    for lon in range(min_lon, max_lon):
        tile_fn = fn % (lat, lon)
        if os.path.isfile(tile_fn):
            print("Skipping {}".format(tile_fn))
            continue
        tile_url = url % tile_fn
        print(tile_url)
        try:
            with urllib.request.urlopen(tile_url) as req:
                with open(tile_fn, "wb+") as f:
                    f.write(req.read())
        except:
            print("Could not download.")
