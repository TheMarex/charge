import React from "react";
import {Scatter,} from "react-chartjs-2";

export default class RouteViewer extends React.Component {
  constructor() {
    super();
    this.chartOptions = {
      showLines: false,
      scales: {
        yAxes: [{
          id: "consumption",
          type: "linear",
          position: "left",
        },],
        xAxes: [{
          id: "duration",
          type: "linear",
          position: "bottom",
        },],
      },
    };
  }

  buildDurationConsumptionDatasets(routes) {
    const colors = {
      "mc_dijkstra": "rgba(255,0,0,0.5)",
      "fp_dijkstra": "rgba(0,255,0,0.5)",
      "fastest_bi_dijkstra": "rgba(0,0,255,0.5)",
      "fpc_dijkstra": "rgba(255,255,0,0.5)",
      "mcc_dijkstra": "rgba(0,255,255,0.5)",
    };
    return routes.map(route => {
      let data = [{x: route.durations[route.durations.length-1], y: route.consumptions[route.consumptions.length-1],},];
      return { yAxisID: "consumption",
        xAxisID: "duration",
        label: route.algorithm,
        data: data,
        borderColor: colors[route.algorithm],};
    });
  }

  render() {
    let datasets = this.buildDurationConsumptionDatasets(this.props.routes);
    let data = {
      datasets: datasets,
    };
    return (
        <Scatter data={data} options={this.chartOptions} width={450} height={300}/>
    );
  }
}
