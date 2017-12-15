import unittest
import numpy as np
import os, sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../src/python")))

from path import *
from graph import Graph
from functions import LinearFunction, PiecewiseFunction
import numeric
import analytic

class TestPathLinking(unittest.TestCase):
    def test_linking_numeric(self):
        T = np.arange(0, 20, 0.1)
        link_consumption = lambda f, g, M: numeric.link_consumption(T, f, g, M)
        link_charging = lambda f, cf, M: numeric.link_charging(T, f, cf, M)
        f = PiecewiseFunction([0], [LinearFunction(0, 5)])
        cf = PiecewiseFunction([0, 10], [LinearFunction(1, 0), LinearFunction(0, 10)])
        graph = Graph([(0, 1, (False, f)), (1, 2, (True, cf))])
        ds, h = link_path(graph, [0, 1, 2], 10, link_consumption, link_charging)
        self.assertEqual(h(0), 5)
        self.assertEqual(h(5), 0)

    def test_linking_analytic(self):
        link_consumption = lambda f, g, M: analytic.link_consumption(f, g)
        link_charging = lambda f, cf, M: analytic.link_charging(f, cf, M)
        f = PiecewiseFunction([0], [LinearFunction(0, 5)])
        cf = PiecewiseFunction([0, 10], [LinearFunction(1, 0), LinearFunction(0, 10)])
        graph = Graph([(0, 1, (False, f)), (1, 2, (True, cf))])
        ds, h = link_path(graph, [0, 1, 2], 10, link_consumption, link_charging)
        self.assertEqual(h(0), 5)
        self.assertEqual(h(5), 0)

if __name__ == '__main__':
    unittest.main()
