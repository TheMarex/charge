## Charge

Charge is a route planner for electric vehicles that uses a realistic consumption model based on driving speed and slope, and a realistic charging model to compute time optimal routes.
Charge uses C++17 and depends on `libosmium` for parsing OSM data in `.osm.pbf` format.
To easily compute and display routes Charge includes a HTTP server based on `web++.hpp`.
The web frontend is based on React and Mapbox GL JS.

## Build

You will need GCC 7 and above to compile `charge`:

```
mkdir build
cd build
cmake ..
make
```

## Data

### Charging stations

We accept charging stations as GeoJSON file with feasures of the following format:

```json
{
    "type": "Feature",
    "properties": {
        "rate": 120000.0
    },
    "geometry": {
        "type": "Point",
        "coordinates": [10.040342, 47.880344]
    }
}
```

Where rate needs to be the charging rate in watt.
We provide a script `scripts/download_tesla.py` that can be used to download Tesla charging station data from their website.

### Elevation data

You can use `./scripts/srtm_download.py` to download SRTM elevation data tiles from OpenTopo and place them in `data/srtm`.

```
mkdir -p data/srtm
# Download SRTM tiles for switzerland
./scripts/srtm_download.py 5,45,11,48 ./data/srtm
```

## Run

Make sure you placed your dataset in `data/graphs/${DATASET}.osm.pbf` and the SRTM tiles in `data/srtm`.
Prepare data and start the server using:

```
DATASET=${DATASET} make run
```

For example to setup OSM switzerland from Geofabrik do:

```
mkdir -p data/graphs
wget http://download.geofabrik.de/europe/switzerland-latest.osm.pbf -O data/graphs/switzerland.osm.pbf
mkdir -p data/srtm
./scripts/srtm_download.py 5,45,11,48 ./data/srtm
CAPACITY=32000 DATASET=switzerland make run
```

### Frontend

![Example Query](example.png)

To start the frontend you will need to have `npm` installed:

```
cd frontend
npm install
npm run start
```

### API Documentation

See [the full HTTP API dpcumentation](docs/API.md).

