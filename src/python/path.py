import numpy as np

from functions import LinearFunction, PiecewiseFunction
from graph import Graph

def link_path(graph, path, M, link_consumption, link_charging, initial_soc=None):
    """Takes graph with data of form (is_charging, weight_function) and link_consumption(f, g, M), link_charging(f, cf, M).
    Returns a tuple (optimal_deltas, consumption)
    """
    if initial_soc is None:
        initial_soc = M
    consumption = PiecewiseFunction([0], [LinearFunction(M - initial_soc, 0)])
    ds = []
    for u, v in zip(path[:-1], path[1:]):
        is_charging, w = graph.data(graph.edge(u, v))
        if is_charging:
            opt_d, consumption = link_charging(consumption, w, M)
        else:
            opt_d, consumption = link_consumption(consumption, w, M)
        ds.append(opt_d)
    return ds, consumption

def get_times(total_time, ds):
    """Returns the time on each path segments.
    >>> get_times(10, [LinearFunction(0, 0), LinearFunction(0, 2), LinearFunction(1, 0), LinearFunction(0, 3)])
    array([2, 1, 0, 7])
    """
    ts = [total_time]
    for d in reversed(ds):
        t = d(ts[-1])
        ts.append(t)

    ts = list(reversed(ts))
    ts = np.array(ts[1:]) - np.array(ts[:-1])
    return ts

if __name__ == "__main__":
    import doctest
    doctest.testmod()
