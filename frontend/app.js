/* eslint-env browser */
/* global mapboxgl */

import React from "react";
import ReactDOM from "react-dom";
import RouteViewer from "./components/route_viewer";
import Overview from "./components/overview";
import * as d3 from "d3-queue";

mapboxgl.accessToken = "pk.eyJ1IjoidGhlbWFyZXgiLCJhIjoiY2l5azA3aTRiMDAwcTJxbzBxMmVzNTMzZiJ9.xitDyEpVTHLl4HmwhJD6nQ";

const map = new mapboxgl.Map({
  container: "map",
  style: "mapbox://styles/themarex/cj7vstqhw3e7h2st4k89wtdu0",
  center: [6.1029052734375, 49.786584231227586,],
  zoom: 12,
});

function obj2url(obj) {
  let url = "?";
  let params = [];
  for (let key in obj)
    {
    params.push(key + "=" + obj[key]);
  }
  return url + params.join("&");
}

function url2obj(url) {
  let params = url.slice(1).split("&");
  let obj = {};
  params.forEach((p) => {
    let kv = p.split("=");
    obj[kv[0]] = kv[1];
  });
  return obj;
}

function state2url(state) {
  let obj = {};
  if (state.startNode)
    obj.startNode = state.startNode.toString();

  if (state.targetNode)
    obj.targetNode = state.targetNode.toString();

  if (state.algorithms)
    obj.algorithms = state.algorithms.join(",");

  obj.toSet = state.toSet;

  return obj2url(obj);
}

function url2state(url) {
  let obj = url2obj(url);
  if (obj.startNode)
    obj.startNode = parseInt(obj.startNode);
  else
    obj.startNode = null;

  if (obj.targetNode)
    obj.targetNode = parseInt(obj.targetNode);
  else
    obj.targetNode = null;

  if (obj.algorithms)
    obj.algorithms = obj.algorithms.split(",");
  else
     obj.algorithms = ["fastest_bi_dijkstra","fpc_dijkstra",];

  obj.selectedLines = [];

  return obj;
}

const state = {
  startCoordinate: null,
  targetCoordinate: null,
  startNode: null,
  targetNode: null,
  showSearchSpace: false,
  toSet: "start",
  selectedLines: [],
  //algorithms: ["mc_dijkstra", "mcc_dijkstra", "fp_dijkstra", "fastest_bi_dijkstra","fpc_dijkstra",],
  algorithms: ["fastest_bi_dijkstra","fpc_dijkstra",],
};

function updateState(newState) {
  let routes_need_update = (newState.startNode && newState.startNode != state.startNode ||
      newState.targetNode && newState.targetNode != state.targetNode);

  let markers_need_update = (newState.startCoordinate && newState.startCoodiante != state.startCoodiante ||
      newState.targetCoordinate && newState.targetCoordinate != state.targetCoordinate);

  for (let key in newState)
    state[key] = newState[key];

  if (routes_need_update)
    updateRoutes();
  if (markers_need_update)
    updateMarker();
}

map.on("load", () => {
  map.addLayer({
    id: "slowdown",
    type: "line",
    source: {
      type: "geojson",
      data: {type: "FeatureCollection", features: [],},
    },
    layout: {
      "visibility": "visible",
      "line-join": "round",
      "line-cap": "round",
    },
    paint: {
      "line-color": {
        "type": "exponential",
        "property": "slowdown",
        "stops": [
          [5, "#ffff00",],
          [30, "#ff0000",],
        ],
      },
      "line-opacity": 0.9,
      "line-width": 8,
    },
  });

  map.addLayer({
    id: "routes",
    type: "line",
    source: {
      type: "geojson",
      data: {type: "FeatureCollection", features: [],},
    },
    layout: {
      "visibility": "visible",
      "line-join": "round",
      "line-cap": "round",
    },
    paint: {
//      "line-color": "#0000BB",
      "line-color": {
        "type": "categorical",
        "property": "algorithm",
        "stops": [
          ["mc_dijkstra", "#ff0000",],
          ["fp_dijkstra", "#00ff00",],
          ["fastest_bi_dijkstra", "#3355ff",],
          ["fpc_dijkstra", "#ffff33",],
          ["mcc_dijkstra", "#00ffff",],
        ],
      },
      "line-opacity": 1.0,
      "line-width": 2,
    },
  });

  map.addLayer({
    id: "marker",
    type: "circle",
    source: {
      type: "geojson",
      data: {
        type: "FeatureCollection",
        features: [],
      },
    },
    paint: {
      "circle-color": {property: "type", type: "categorical", stops: [["start", "#BBBBBB",], ["target", "#333333",],],},
      "circle-stroke-color": "#FFFFFF",
      "circle-radius": 6,
      "circle-stroke-width": 2,
      "circle-stroke-opacity": 0.9,
      "circle-opacity": 0.8,
    },
  });

  map.addLayer({
    id: "search_space",
    type: "circle",
    source: {
      type: "geojson",
      data: {
        type: "FeatureCollection",
        features: [],
      },
    },
    paint: {
      "circle-color": {property: "labels", type: "exponential", stops: [[0, "#00FF00",], [20, "#FF0000",],],},
      "circle-radius": 2,
      "circle-opacity": 0.2,
    },
  });

  map.on("click", (data) => {
    if (state.selectedLines.length > 0) {
      let line = state.selectedLines[0];
      ReactDOM.render(
        <RouteViewer heights={JSON.parse(line.properties.heights)}
                 lengths={JSON.parse(line.properties.lengths)}
                 consumptions={JSON.parse(line.properties.consumptions)}
                 durations={JSON.parse(line.properties.durations)}/>,
        document.getElementById("sidebar")
      );

      document.getElementById("sidebar").classList.remove("hidden");
    } else if (state.toSet == "start") {
      updateState({"startCoordinate":  data.lngLat.toArray(),
        "toSet": "target",});
      requestSnap(state.startCoordinate, function() {
        const resp = onNearest.call(this);
        updateState({"startNode":  resp.id,});
      });
    } else if (state.toSet == "target") {
      updateState({"targetCoordinate":  data.lngLat.toArray(),
        "toSet": "start",});
      requestSnap(state.targetCoordinate, function() {
        const resp = onNearest.call(this);
        updateState({"targetNode":  resp.id,});
      });
    }
  });

  map.on("mousemove", (e) => {
    const features = map.queryRenderedFeatures(e.point, {layers: ["routes",],});
    if (features) {
      let selected = features.filter((f) => { return f.properties.type == "route";});
      if (selected.length != state.selectedLines.length)
        updateState({"selectedLines": selected,});

      let cursor = state.selectedLines.length > 0 ? "pointer" : "";

      map.getCanvas().style.cursor = cursor;
    }
  });

  updateState(url2state(window.location.search));
});


function makePoint(position, type) {
  return {
    type: "Feature",
    geometry: {
      type: "Point",
      coordinates: position,
    },
    properties: {
      "type": type,
    },
  };
}

function updateMarker() {
  const collection = {type: "FeatureCollection", features: [],};
  if (state.startCoordinate) {
    collection.features.push(makePoint(state.startCoordinate, "start"));
  }
  if (state.targetCoordinate) {
    collection.features.push(makePoint(state.targetCoordinate, "target"));
  }
  map.getSource("marker").setData(collection);
}

function updateRoutes() {
  if (state.startNode != null && state.targetNode != null) {
    window.history.pushState(state, "route", state2url(state));
    let q = d3.queue();
    state.algorithms.forEach((a) => {
      q.defer(requestRoute, state.startNode, state.targetNode, a);
    });

    q.awaitAll((err, responses) => {
      if (err) {
        console.error("Error querying routes" + err);
        return;
      }

      let routes = [];
      responses.forEach((r) => {
        const {algorithm, response,} = r;
        let route = response.routes[0]; // only display one solution
        route.algorithm = algorithm;
        routes.push(route);
      });
      updateRouteLines(routes);
      updateSlowdown(routes);
      state.startCoordinate = routes[0].geometry[0];
      state.targetCoordinate = routes[0].geometry[routes[0].geometry.length-1];
      updateMarker();

      let bounds = new mapboxgl.LngLatBounds(state.startCoordinate, state.startCoordinate);
      bounds.extend(state.targetCoordinate);
      routes[0].geometry.forEach(c => bounds.extend(c));
      map.fitBounds(bounds, {padding: 100, maxZoom:10,});
    });
  }
}

function onNearest() {
  const res = JSON.parse(this.responseText);
  if (res.error) {
    console.error(res.error);
    return;
  }

  return res;
}

function updateSlowdown(routes) {
  let make_feature = (slowdown, geometry) => {
    return {
      type: "Feature",
      properties: { slowdown: slowdown,},
      geometry: {
        type: "LineString",
        coordinates: geometry,
      },
    };
  };

  let features = [];

  routes.forEach(route => {
    if (!route.max_speeds && route.max_speeds.length == 0)
      return;
    for(let i = 0; i < route.durations.length-1; ++i) {
      let duration = route.durations[i+1] - route.durations[i];
      let length = route.lengths[i+1] - route.lengths[i];
      let max_speed = route.max_speeds[i];
      let speed = length / duration * 3.6;
      let geometry = [route.geometry[i], route.geometry[i+1],];

      if (max_speed - speed > 5)
    {
        features.push(make_feature(max_speed - speed, geometry));
      }
    }
  });

  const fc = {
    type: "FeatureCollection",
    features: features,
  };

  map.getSource("slowdown").setData(fc);
}

function updateRouteLines(routes) {
  let features = routes.map((route) => {
    return {
      type: "Feature",
      properties: {
        consumption: route.consumptions[route.consumptions.length-1],
        heights:route.heights,
        consumptions: route.consumptions,
        durations: route.durations,
        lengths: route.lengths,
        algorithm: route.algorithm,
        type: "route",},
      geometry: {
        type: "LineString",
        coordinates: route.geometry,
      },
    };
  });
  const fc = {
    type: "FeatureCollection",
    features: features,
  };

  ReactDOM.render(
    <Overview routes={routes}/>,
    document.getElementById("sidebar")
  );
  document.getElementById("sidebar").classList.remove("hidden");

  map.getSource("routes").setData(fc);

  if (state.showSearchSpace) {
    routes.map((route) => {
      if (route.search_space) {
        map.getSource("search_space").setData(route.search_space);
      }
    });
  }

}

function requestSnap(coord, callback) {
  const oReq = new XMLHttpRequest();
  oReq.addEventListener("load", callback);
  oReq.open("GET", `//127.0.0.1:5000/nearest?lon=${coord[0]}&lat=${coord[1]}`);
  oReq.send();
}

function requestRoute(start, target, algorithm, callback) {
  fetch(`//127.0.0.1:5000/route?algorithm=${algorithm}&start=${start}&target=${target}&search_space=true`)
    .then((response) => {
      if (response.ok)
        return response.json();
    }).then((json) => { callback(null, {algorithm: algorithm, response: json,}); })
    .catch((err) => { callback(err); });
}
