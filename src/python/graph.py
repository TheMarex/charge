
from functions import LinearFunction

class Graph:
    def __init__(self, edges):
        """
        >>> graph = Graph([(0, 1, LinearFunction(0, 5))])
        >>> graph.target(graph.edge(0, 1))
        1
        >>> graph.data(graph.edge(0, 1)).params()
        (0, 5)
        """
        edges = sorted(edges, key=lambda e: (e[0], e[1]))

        self._targets = [target for _, target, _ in edges]
        self._data = [data for _, _, data in edges]

        self._first_out = []
        idx = 0
        while idx < len(edges):
            source = edges[idx][0]
            begin = idx
            while idx < len(edges) and edges[idx][0] == source:
                idx += 1
            self._first_out.append(begin)
        self._first_out.append(len(edges))

    def data(self, edge):
        return self._data[edge]

    def target(self, edge):
        return self._targets[edge]

    def edge(self, u, v):
        for edge in self.edges(u):
            if self._targets[edge] == v:
                return edge
        return None

    def edges(self, u):
        begin = self._first_out[u]
        end = self._first_out[u+1]
        for edge in range(begin, end):
            yield edge

if __name__ == "__main__":
    import doctest
    doctest.testmod()
