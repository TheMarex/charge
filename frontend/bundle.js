/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, {
/******/ 				configurable: false,
/******/ 				enumerable: true,
/******/ 				get: getter
/******/ 			});
/******/ 		}
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 0);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports) {

/* eslint-env browser */
/* global mapboxgl */

mapboxgl.accessToken = "pk.eyJ1IjoidGhlbWFyZXgiLCJhIjoiY2l5azA3aTRiMDAwcTJxbzBxMmVzNTMzZiJ9.xitDyEpVTHLl4HmwhJD6nQ";

const map = new mapboxgl.Map({
  container: "map",
  style: "mapbox://styles/mapbox/dark-v9",
  center: [6.1029052734375, 49.786584231227586,],
  zoom: 12,
});


const state = {
  startCoordinate: null,
  targetCoordinate: null,
  startNode: null,
  targetNode: null,
  toSet: "start",
};
const popup = new mapboxgl.Popup({closeButton: false, closeOnClick: false,});

map.on("load", () => {
  map.addLayer({
    id: "unpacked",
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
      "line-color": "#0000BB",
      "line-opacity": 0.5,
      "line-width": 5,
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
      "circle-color": {property: "type", type: "categorical", stops: [["start", "#FFFFFF",], ["target", "#AAAAAA",],],},
      "circle-radius": 8,
      "circle-opacity": 0.8,
    },
  });

  map.on("click", (data) => {
    if (state.toSet == "start") {
      state.startCoordinate = data.lngLat.toArray();
      state.toSet = "target";
      requestSnap(state.startCoordinate, function() {
        const resp = onNearest.call(this);
        state.startNode = resp.id;
        updateRoute();
      });
    } else if (state.toSet == "target") {
      state.targetCoordinate = data.lngLat.toArray();
      state.toSet = "start";
      requestSnap(state.targetCoordinate, function() {
        const resp = onNearest.call(this);
        state.targetNode = resp.id;
        updateRoute();
      });
    }

    updateMarker();
  });

  map.on("mousemove", (e) => {
    const features = map.queryRenderedFeatures(e.point, {layers: ["unpacked",],});
    if (!features) return;


    if (!features.length) {
      popup.remove();
      return;
    }

    const line = features.filter((f) => { return f.properties.type == "route";});

    if (line) {
      let description = `<strong>Costs:</strong> ${line.properties.costs}`;

        // Populate the popup and set its coordinates
        // based on the feature found.
      popup.setLngLat(map.unproject(e.point))
            .setHTML(description)
            .addTo(map);
    }

    let cursor = features.length ? "pointer" : "";

  // Change the cursor style as a UI indicator.
    map.getCanvas().style.cursor = cursor;
  });

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

function updateRoute() {
  if (state.startNode != null && state.targetNode != null) {
    requestRoute(state.startNode, state.targetNode, onRoute);
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

function onRoute() {
  const res = JSON.parse(this.responseText);
  if (res.error) {
    console.error(res.error);
    return;
  }

  const featuresUnpacked = {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: {costs: res.costs, path: res.path, type: "route",},
        geometry: {
          type: "LineString",
          coordinates: res.geometry,
        },
      },],
  };

  map.getSource("unpacked").setData(featuresUnpacked);
}

function requestSnap(coord, callback) {
  const oReq = new XMLHttpRequest();
  const str_coord = coord.join(",");
  oReq.addEventListener("load", callback);
  oReq.open("GET", `//127.0.0.1:5000/nearest?lon=${coord[0]}&lat=${coord[1]}`);
  oReq.send();
}

function requestRoute(start, target, callback) {
  const oReq = new XMLHttpRequest();
  oReq.addEventListener("load", callback);
  oReq.open("GET", `//127.0.0.1:5000/route?algorithm=fastest_bi_dijkstra&start=${start}&target=${target}`);
  oReq.send();
}


/***/ })
/******/ ]);