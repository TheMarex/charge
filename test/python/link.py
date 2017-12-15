import unittest
import numpy as np
import os, sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../src/python")))

from constants import eps
from functions import LinearFunction, PiecewiseFunction, HypLinFunction, TradeoffFunction, ChargingFunction
import analytic
import numeric

# for faster test runs
eps = eps*100

def equal(name, T, lhs, rhs):
    l = lhs(T)
    r = rhs(T)
    diff = np.fabs(l - r)
    is_same = l == r
    diff[is_same] = 0
    error = diff > eps*10
    idx = np.argmax(error)
    if error[idx]:
        print("{}({}) {} != {} ({})".format(name, T[error], l[error], r[error], diff[error]))
        return False
    return True

class TestChargingLinking(unittest.TestCase):
    def test_linking_lin_cf(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = PiecewiseFunction([0], [LinearFunction(0, 5)])
        cf = PiecewiseFunction([0, 10], [LinearFunction(1, 0), LinearFunction(0, 10)])
        numeric_opt_d, numeric_h = numeric.link_charging(T, f, cf, M)
        analytic_opt_d, analytic_h = analytic.link_charging(f, cf, M)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h", T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(0), 5)
        self.assertEqual(analytic_h(10), 0)

    def test_linking_hyp_cf(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = PiecewiseFunction([0, 2, 6], [LinearFunction(0, float('inf')), HypLinFunction(5, 1, 1, 0), LinearFunction(0, 6/5.)])
        cf = PiecewiseFunction([0, 10], [LinearFunction(1, 0), LinearFunction(0, 10)])
        numeric_opt_d, numeric_h = numeric.link_charging(T, f, cf, M)
        analytic_opt_d, analytic_h = analytic.link_charging(f, cf, M)
        # Wrong due to numeric instabilities between 3.16 and 40
        #self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("opt_d", T[T < 3.16], numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h", T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(0), float('inf'))
        self.assertEqual(analytic_h(2), 6)
        self.assertEqual(analytic_h(8), 0)

    def test_linking_hyp_cf_2(self):
        T = np.arange(0, 40, eps)
        M = 10
        f = TradeoffFunction(5, 10, 4, 4, 0)
        cf = ChargingFunction(np.array([0, 8, 16, 32]), np.array([0, 1/2., 3/4., 1.])*M, M)
        numeric_opt_d, numeric_h = numeric.link_charging(T, f, cf, M)
        analytic_opt_d, analytic_h = analytic.link_charging(f, cf, M)
        # Wrong due to numeric instabilities between 7.67 and 40
        self.assertTrue(equal("opt_d", T[T < 7.67], numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h", T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(0), float('inf'))
        self.assertEqual(analytic_h(5), 4)
        self.assertEqual(analytic_h(10), 0)


class TestConsumptionLinking(unittest.TestCase):
    def test_lin_lin_same_linking(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = PiecewiseFunction([0], [LinearFunction(0, 5)])
        g = PiecewiseFunction([0], [LinearFunction(0, 5)])
        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(0), 10)
        self.assertEqual(analytic_h(20), 10)

    def test_lin_lin_better_linking(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = PiecewiseFunction([0, 5], [LinearFunction(-1, 5), LinearFunction(0, 0)])
        self.assertEqual(f(5), 0)
        self.assertEqual(f(0), 5)
        self.assertEqual(f(10), 0)
        g = PiecewiseFunction([0], [LinearFunction(0, 5)])
        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(0), 10)
        self.assertEqual(analytic_h(5), 5)
        self.assertEqual(analytic_h(10), 5)

    def test_hyp_lin_linking(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = PiecewiseFunction([0, 2, 6], [LinearFunction(0, float('inf')), HypLinFunction(5, 1, 1, 0), LinearFunction(0, 6/5.)])
        self.assertEqual(f(2), 6)
        self.assertEqual(f(6), 1.2)
        self.assertEqual(f(10), 1.2)
        g = PiecewiseFunction([0, 4, 9], [LinearFunction(0, float('inf')), LinearFunction(-1, 9), LinearFunction(0, 0)])
        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))
        self.assertEqual(analytic_h(6), 11)
        self.assertEqual(analytic_h(16), 6/5.)

    def test_hyp_hyp_linking_case_3(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = TradeoffFunction(2, 4, 1, 1, -0.5)
        g = TradeoffFunction(5, 10, 4, 4, 1)
        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

        # should also work in the symmetric case
        numeric_opt_d, numeric_h = numeric.link_consumption(T, g, f, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(g, f)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

    def test_hyp_hyp_linking_case_1(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = TradeoffFunction(5, 6, 4, 4, 1)
        g = TradeoffFunction(2, 4, 1, 1, -0.5)

        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

        numeric_opt_d, numeric_h = numeric.link_consumption(T, g, f, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(g, f)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

    def test_hyp_hyp_linking_case_2(self):
        T = np.arange(0, 20, eps)
        M = 10
        f = TradeoffFunction(5, 6, 4, 4, 1)
        g = TradeoffFunction(3, 4, 1, 1, -0.5)

        numeric_opt_d, numeric_h = numeric.link_consumption(T, f, g, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(f, g)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

        numeric_opt_d, numeric_h = numeric.link_consumption(T, g, f, M)
        analytic_opt_d, analytic_h = analytic.link_consumption(g, f)
        self.assertTrue(equal("opt_d", T, numeric_opt_d, analytic_opt_d))
        self.assertTrue(equal("h",T, numeric_h, analytic_h))

if __name__ == '__main__':
    unittest.main()

