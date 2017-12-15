import React from "react";
import numeral from "numeral";
import {Line,} from "react-chartjs-2";

export default class RouteViewer extends React.Component {
  constructor() {
    super();
    this.chartOptions = {
      elements: {
        line: {
          tension: 0,
        },
      },
      scales: {
        yAxes: [{
          id: "consumption",
          type: "linear",
          position: "left",
        },
        {
          id: "height",
          type: "linear",
          position: "right",
          ticks: { min: 0,},
        },],
        xAxes: [{
          type: "linear",
          position: "bottom",
        },],
      },
    };
  }

  formatDuration(duration) {
    return numeral(duration).format("00:00:00");
  }

  formatConsumption(consumption) {
    return numeral(consumption).format("0,0") + " Wh";
  }

  formatLength(length) {
    return numeral(length).format("0,0") + " m";
  }

  buildConsumptionDataset(lengths, consumptions) {
    let data = [];
    for (let idx = 0; idx < lengths.length; ++idx) {
      data.push({x: lengths[idx], y: consumptions[idx],});
    }
    return { yAxisID: "consumption", label: "Consumption", data: data, borderColor: "rgba(255,255,0,0.5)",};
  }

  buildHeightDataset(lengths, heights) {
    let data = [];
    for (let idx = 0; idx < lengths.length; ++idx) {
      data.push({x: lengths[idx], y: heights[idx],});
    }
    return { yAxisID: "height", label: "Height", data: data, borderColor: "rgba(255,0,0,0.5)",};
  }

  render() {
    let datasets = [
      this.buildConsumptionDataset(this.props.durations, this.props.consumptions),
      this.buildHeightDataset(this.props.durations, this.props.heights),
    ];
    let data = {
      datasets: datasets,
    };
    return (
            <div>
            <div id='header'>
                <h3>duration: {this.formatDuration(this.props.durations[this.props.durations.length-1])}</h3>
                <h3>consumption: {this.formatConsumption(this.props.consumptions[this.props.consumptions.length-1])}</h3>
                <h3>length: {this.formatLength(this.props.lengths[this.props.lengths.length-1])}</h3>
            </div>
            <div id='charts'>
                <Line data={data} options={this.chartOptions} width={450} height={300}/>
            </div>
            </div>
    );
  }
}
