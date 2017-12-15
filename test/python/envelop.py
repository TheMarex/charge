import unittest
import numpy as np
import os, sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../src/python")))

from functions import LinearFunction, PiecewiseFunction, HypLinFunction, lower_envelop


class TestEnvelop(unittest.TestCase):
    def test_envelop_regression_1(self):
        key = lambda interval, x: interval[3](x)
        intervals = [(5, 10, LinearFunction(1, 0), HypLinFunction(4, 4, 0, 0)),\
                     (10, float('inf'), LinearFunction(1, 0), LinearFunction(0, 0.1111111111111111)),\
                     (7.7132710668902229, 9.5699066003353366, LinearFunction(0, 7.713271066890223), LinearFunction(-0.15625, 1.4952979063023975)),\
                     (9.5699066003353366, float('inf'), LinearFunction(0, 7.713271066890223), LinearFunction(-0.0, 0.0)),\
                     (5, 9.7999999999999972, LinearFunction(0, 5), LinearFunction(-0.3125, 5.562499999999999)),\
                     (9.7999999999999972, 25.799999999999997, LinearFunction(0, 5), LinearFunction(-0.15625, 4.03125)),\
                     (25.799999999999997, float('inf'), LinearFunction(0, 5), LinearFunction(-0.0, 0.0)),\
                     (10, 10.711111111111109, LinearFunction(0, 10), LinearFunction(-0.15625, 1.6736111111111107)),\
                     (10.711111111111109, float('inf'), LinearFunction(0, 10), LinearFunction(-0.0, 0.0))]
        result = lower_envelop(intervals, key)
        reference = [\
            (5, 7.713271066890223, 0),
            (25.08888888888889, 25.799999999999997, 3),
            (7.713271217579087, 9.569906600335337, 2),
            (7.713271066890223, 7.713271217579087, 2),
            (9.799999999999997, 10, 3),
            (10, 10.711111111111109, 3),
            (9.569906600335337, 9.799999999999997, 3),
            (25.799999999999997, float('inf'), 3),
            (10.711111111111109, 25.08888888888889, 3)]
        self.assertSetEqual(set(result), set(reference))

    def test_envelop_regression_2(self):
        key = lambda interval, x: interval[3](x)
        intervals = [\
                (5.0000000000000018, float('inf'), LinearFunction(0, 3.0000000000000018), LinearFunction(0, 6.999999999999986)),\
                (6.5999999999999996, float('inf'), LinearFunction(0, 4.6), LinearFunction(0, 3.5917159763313613)),\
                (6.5999999999999996, 7.7132710668902229, LinearFunction(1, -2), HypLinFunction(4, 4, 3.0, 0)),\
                (7.7132710668902229, float('inf'), LinearFunction(0, 5.713271066890223), LinearFunction(0, 3.2900993021007987)),\
                (7.71327115864805, float('inf'), LinearFunction(0, 5.71327115864805), LinearFunction(0, 3.290099287763639)),\
                (7.71327115864805, 9.0, LinearFunction(1, -2), LinearFunction(-0.15625, 4.495297906302397)),\
                (9.0, float('inf'), LinearFunction(0, 7.0), LinearFunction(0, 3.0890479063023966)),\
                (9.0, 14.6, LinearFunction(1, -2), LinearFunction(-0.15625, 4.495297906302397)),\
                (14.6, float('inf'), LinearFunction(0, 12.6), LinearFunction(0, 2.2140479063023966)),\
                (14.6, 16.776, LinearFunction(1, -2), LinearFunction(-0.15625, 4.495297906302397)),\
                (16.776, float('inf'), LinearFunction(0, 14.776), LinearFunction(0, 1.8740479063023967)),\
                (16.776, 22.369906600335334, LinearFunction(1, -2), LinearFunction(-0.15625, 4.495297906302397)),\
                (22.369906600335334, float('inf'), LinearFunction(0, 20.369906600335334), LinearFunction(0, 1.0000000000000009)),\
                (22.369906600335334, float('inf'), LinearFunction(0, 20.369906600335334), LinearFunction(0, 1.0)),\
                (22.823999999999998, float('inf'), LinearFunction(0, 20.823999999999998), LinearFunction(0, 1.0)),\
                (30.600000000000001, float('inf'), LinearFunction(0, 28.6), LinearFunction(0, 1.0))]
        result = lower_envelop(intervals, key)
        reference = [\
            (14.6, 16.776, 9),
            (16.776, 22.369906600335334, 11),
            (30.6, float('inf'), 12),
            (6.6, 7.713271066890223, 2),
            (22.823999999999998, 30.6, 12),
            (9.0, 14.6, 7),
            (22.369906600335334, 22.823999999999998, 12),
            (5.000000000000002, 6.6, 0),
            (7.713271066890223, 9.0, 5)]

        self.assertSetEqual(set(result), set(reference))

if __name__ == '__main__':
    unittest.main()
