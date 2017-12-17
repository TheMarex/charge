# HTTP API

The HTTP API is implemented over a REST scheme where HTTP GET requests return JSON objects.
The general URL scheme is:

```
http://{host}/{service}?{param}={value}
```

Where service can be either `nearest` for the Nearest service to snap coordinates to nodes, or `route` for the actual Route service.

## Nearest

Returns the ID and coordinate of the nearest node in the road network.

### Query

#### Parameters

|param|value|
|-----|-----|
|`lon`|Longitude of the coordinate|
|`lat`|Latitude of the coordinate|

#### Example

```
http://{host}/nearest?lon=8.419845700263977&lat=49.014124567148194
```

### Response

#### Response JSON object

- `id`: Integer that contains the node ID
- `coordiante`: Array of two floating point values that represent the snapped coordinates in `[{lon}, {lat}]` encoding.

#### Example

```json
{"id": 1,  "coordinate": [8.419845700263977, 49.014124567148194]}
```

## Route

Return the fastest route of an electric vehicle between two nodes using a specific algorithm.

### Query

#### Parameters

|param|value|
|-----|-----|
|`start`|Node ID of the start|
|`target`|Node ID of the target|
|`algorithm`|One of `fpc_dijkstra`, `bi_fastest_dijkstra`, `mcc_dijkstra`, `fp_dijkstra`, `mc_dijkstra`|
|`search\_space`|

#### Example

```
http://{host}/route?start=1&target=2&algorithm=fpc_dijkstra
```

### Response

#### Response JSON object

- `start`: Integer that encodes the node ID of the start
- `target`: Integer that encodes the node ID of the target
- `routes`: Array of `Route` objects.

### `Route` JSON object

- `path`: Array of node ids that make up the route.
- `durations`: Array of durations values in `sec` for every point on the path (accumulative).
- `consumptions`: Array of consumption values in `Wh` for every point on the path (accumulative).
- `lengths`: Array of distance values in `meter` for every point on the path (accumulative).
- `heights`: Array of height values in `meter` for every point on the path.
- `max_speeds`: Array of maximum speed values in `km/h` for every segment. `max_speeds[i]` corresponds to the segment `path[i-1], path[i]`. The first values is always 0.
- `geometry`: Array of coordinates encodes as arrays in `[{lon}, {lat}]` order.
- `search_space`: (optional) GeoJSON feature collection of the search space of the algorithm.
- `tradeoff`: Array of `Tradeoff` objects that represent sub-functions of the optimal time-consumption trade-off function

### `Tradeoff` JSON object

Encodes parameters for the formula: `a/(x-b)^2 + c + d * (x - b)`, where `x` is the duration and the result the consumption in `Wh`.

- `min_duration`: Minimal travel time in `sec`.
- `max_duration`: Maximal travel time in `sec`.
- `a`: Floating number indicating the first parameter of the hyperbolic function
- `b`: Floating number indicating the second parameter of the hyperbolic function
- `c`: Floating number indicating the third parameter of the hyperbolic function
- `d`: Floating number indicating the fourth parameter of the hyperbolic function
