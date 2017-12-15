import React from "react";

export default class Options extends React.Component {
  constructor(props) {
    super(props);
    this.state = props.state;
  }

  toggleAlgorithm(algorithm) {
    let index = this.state.algorithms.indexOf(algorithm);
    if (index >= 0)
    {
      this.state.algorithms = this.state.algorithms.splice(index, 1);
    }
    else
    {
      this.state.algorithms.push(algorithm);
    }
  }

  render() {
    const useMC = this.state.algorithms.indexOf("mc_dijkstra") >= 0;
    const useFP = this.state.algorithms.indexOf("fp_dijkstra") >= 0;
    const useDijkstra = this.state.algorithms.indexOf("fastest_bi_dijkstra") >= 0;
    return(
          <form>
            <label>
              <input type="checkbox" id="mc_dijkstra_button" checked={useMC} onChange={() => { this.toggleAlgorithm("mc_dijkstra"); }}/>
              MC Dijkstra
            </label><br/>
            <label>
              <input type="checkbox" id="fp_dijkstra_button" checked={useFP} onChange={() => { this.toggleAlgorithm("fp_dijkstra"); }}/>
              FP Dijkstra
            </label><br/>
            <label>
              <input type="checkbox" id="fastest_bi_dijkstra_button" checked={useDijkstra} onChange={() => { this.toggleAlgorithm("fastest_bi_dijkstra"); }}/>
               Min-Duration Dijkstra
            </label>
          </form>
    );
  }
}
