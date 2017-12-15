import numpy as np
import math

from constants import *
from utils import deg3_real_roots, intersect, get_sorting

_apply_vectorized = np.vectorize(lambda f, x: f(x), otypes=[np.dtype('float')])

class PiecewiseFunction:
    def __init__(self, xs, fns):
        self._xs = np.array(xs)
        self._fns = np.array(fns)
        assert(len(self._xs) > 0)
        assert(len(self._fns) > 0)
        if len(self._xs) == len(self._fns)+1:
            self._fns = np.concatenate((self._fns, np.array([LinearFunction(0, self._fns[-1](self._xs[-1]))])))
        assert(len(self._xs) == len(self._fns))
        assert(np.all([callable(f) for f in self._fns]))

    def __get_interval_index(self, values, v):
        idxs = np.full_like(v, 0, dtype='int32')
        for idx in range(len(values)):
            idxs[v >= values[idx]] = idx
        return idxs

    def __call__(self, x):
        is_scalar = isinstance(x, float) or isinstance(x, int) or isinstance(x, np.int64) or isinstance(x, np.int32)
        if is_scalar:
            x = np.array([x])

        xidxs = self.__get_interval_index(self._xs, x)
        y = _apply_vectorized(self._fns[xidxs], x)

        if is_scalar:
            return y[0]
        else:
            return y

    def __repr__(self):
        return "PiecewiseFunction(" + str(list(self._xs)) + "," + str(list(self._fns)) + ")"

    def diff(self):
        fns_diff = [f.diff() for f in fns[1:-1]]
        return PiecewiseFunction(self._xs, fns_diff)

    def domains(self):
        return zip(self._xs, self._fns)

    def intervals(self):
        intervals = [(x_min, x_max, fn) for x_min, x_max, fn in zip(self._xs[:-1], self._xs[1:], self._fns)]
        intervals.append((self._xs[-1], float('inf'), self._fns[-1]))
        return intervals

class LinearFunction:
    def __init__(self, d, c, b=0):
        self._a = d
        self._b = c - d*b

    def __call__(self, x):
        is_scalar = isinstance(x, float) or isinstance(x, int) or isinstance(x, np.int64) or isinstance(x, np.int32)
        if is_scalar:
            x = np.array([x], dtype=np.float)
        ys = None
        if self._a == 0.0:
            ys = np.full_like(x, self._b)
        else:
            ys = self._a*x + self._b
        if is_scalar:
            return ys[0]
        else:
            return ys

    def __repr__(self):
        return "LinearFunction({}, {})".format(self._a, self._b)

    def inverse(self, domain=[float('-inf'), float('inf')]):
        if self._a > 0 or self._a < 0:
            return LinearFunction(1. / self._a, - self._b / self._a)
        else:
            return LinearFunction(0, domain[0])

    def diff(self):
        return LinearFunction(d=0, c=self._a)

    def params(self):
        return (self._a, self._b)

class InvHypFunction:
    def __init__(self, a, b, c, domain=[float('-inf'), float('inf')]):
        self._a = a
        self._b = b
        self._c = c
        self._domain = domain

    def __repr__(self):
        return "InvHypFunction({}, {}, {}, domain={})".format(self._a, self._b, self._c, self._domain)

    # assumes x >= b
    def __call__(self, y):
        is_scalar = isinstance(x, float) or isinstance(x, int) or isinstance(x, np.int64) or isinstance(x, np.int32)
        if is_scalar:
            ys = np.array([y])
        else:
            ys = np.array(y)
        if np.any(ys < self._c):
            raise Exception("Trying to invert an invalid value {}".format(ys[ys < self._c]))
        xs = np.full_like(ys, self._domain[1])
        xs[ys > self._c] = self._b + np.sqrt(self._a / (ys[ys > self._c] - self._c))
        if is_scalar:
            return xs[0]
        else:
            return xs

    def inverse(self):
        return HypLinFunction(self._a, self._b, self._c, 0)

class HypLinFunction:
    def __init__(self, a, b, c, d):
        assert(a > 0)
        assert(np.isfinite(a))
        assert(np.isfinite(b))
        assert(np.isfinite(c))
        assert(np.isfinite(d))
        self._a = a
        self._b = b
        self._c = c
        self._d = d

    def __call__(self, x):
        if math.fabs(self._a) < eps:
            if math.fabs(self._d) < eps:
                return self._c
            else:
                return self._c + self._d*(x - self._b)
        else:
            return self._a / (x - self._b)**2 + self._c + self._d*(x - self._b)

    def __repr__(self):
        return "HypLinFunction({}, {}, {}, {})".format(self._a, self._b, self._c, self._d)

    def inverse(self, domain=[float('-inf'), float('inf')]):
        if self._d < 0 or self._d > 0:
            if self._a > 0. or self._a < 0:
                raise Exception("HypLinFunction should not occur")
            else:
                return LinearFunction(self._c, self._d).inverse()
        else:
            return InvHypFunction(self._a, self._b, self._c, domain=domain)


    def diff(self):
        return lambda x, a=self._a, b=self._b, d=self._d: -2 * a / (x - b)**3 + d

    def params(self):
        return (self._a, self._b, self._c, self._d)

class ConstantFunction(PiecewiseFunction):
    def __init__(self, t_min, c):
        f = LinearFunction(0, c)
        super().__init__([0, t_min], [LinearFunction(0, float('inf')), f])

class TradeoffFunction(PiecewiseFunction):
    def __init__(self, t_min, t_max, a, b, c):
        assert(math.fabs(a) > 0)
        hyp = HypLinFunction(a, b, c, 0)
        super().__init__([0, t_min, t_max], [LinearFunction(0, float('inf')), hyp, LinearFunction(0, hyp(t_max))])

class ChargingFunction(PiecewiseFunction):
    def __init__(self, ts, ys, M):
        assert(ys[0] == 0)
        assert(ys[-1] == M)
        fns = np.concatenate((make_piecewise_linear(ts, ys),  [LinearFunction(0., ys[-1])]))
        super().__init__(ts, fns)

    def inverse(self):
        return invert_piecewise_linear(self)

def make_piecewise_linear(xs, ys):
    dx = xs[1:]-xs[:-1]
    dy = ys[1:]-ys[:-1]
    dy[ys[1:] == ys[:-1]] = 0
    A = dy/dx
    B = ys[:-1] - A*xs[:-1]

    index = A >= float('inf')
    A[index] = 0
    B[index] = ys[:-1][index]

    index = A <= -float('inf')
    A[index] = 0
    B[index] = ys[:-1][index]

    fns = []
    for a,b in zip(A,B):
        fns.append(LinearFunction(a, b))
    return fns

def invert_piecewise_linear(f):
    ys = [sub_f(x_min) for x_min, sub_f in f.domains()]
    asc, dsc = get_sorting(list(ys))
    if not asc and not dsc:
        raise Exception("Not monotone")

    if asc:
        inv_domains = []
        for i, (x_min, x_max, sub_f) in enumerate(f.intervals()):
            y_min = ys[i]
            inv_sub_f = sub_f.inverse(domain=[x_min, x_max])
            inv_domains.append((y_min, inv_sub_f))
    else:
        assert(dsc)
        inv_domains = []
        for i, (x_min, x_max, sub_f) in enumerate(f.intervals()):
            y_min = sub_f(x_max)
            inv_sub_f = sub_f.inverse(domain=[x_min, x_max])
            inv_domains.append((y_min, inv_sub_f))
        inv_domains = list(reversed(inv_domains))

    xs = [x for x, _ in inv_domains]
    fns = [f for _, f in inv_domains]
    return PiecewiseFunction(xs, fns)



# positive value shifts the function to the left
# negative value to the right
def shift(f, x_shift):
    assert(isinstance(f, PiecewiseFunction))
    new_xs = []
    new_fns = []
    for x_min, sub_f in f.domains():
        assert(isinstance(sub_f, LinearFunction))
        a, b = sub_f.params()
        b = b + a*x_shift
        new_xs.append(x_min - x_shift)
        new_fns.append(LinearFunction(a, b))
    return PiecewiseFunction(new_xs, new_fns)

def clip(f, x_0):
    assert(isinstance(f, PiecewiseFunction))
    new_xs = []
    new_fns = []
    for x_min, x_max, sub_f in f.intervals():
        new_x_min = max(x_min, x_0)
        if new_x_min < x_max:
            new_xs.append(new_x_min)
            new_fns.append(sub_f)
    if len(new_fns) > 0:
        return PiecewiseFunction(new_xs, new_fns)
    else:
        return None

def multiply(f, c):
    assert(isinstance(f, PiecewiseFunction))
    new_xs = []
    new_fns = []
    for x_min, x_max, sub_f in f.intervals():
        assert(isinstance(sub_f, LinearFunction))
        a, b = sub_f.params()
        new_xs.append(x_min)
        new_fns.append(LinearFunction(c*a, c*b))
    return PiecewiseFunction(new_xs, new_fns)

def offset(f, c):
    assert(isinstance(f, PiecewiseFunction))
    new_xs = []
    new_fns = []
    for x_min, x_max, sub_f in f.intervals():
        assert(isinstance(sub_f, LinearFunction))
        a, b = sub_f.params()
        new_xs.append(x_min)
        new_fns.append(LinearFunction(a, c+b))
    return PiecewiseFunction(new_xs, new_fns)

# Intersects two functions f_1 and f_2
# The assumption is that they are either
# hyperbolic or linear
def intersect_functions(f_1, f_2, domain):
    x_min, x_max = domain
    if len(f_1.params()) == 4:
        a_1, b_1, c_1, d_1 = f_1.params()
    elif len(f_1.params()) == 2:
        d_1, c_1 = f_1.params()
        a_1 = 0
        b_1 = 0

    if len(f_2.params()) == 4:
        a_2, b_2, c_2, d_2 = f_2.params()
    elif len(f_2.params()) == 2:
        d_2, c_2 = f_2.params()
        a_2 = 0
        b_2 = 0

    intersections = []
    if a_1 == 0 and a_2 == 0:
        if math.fabs(d_1 - d_2) > eps:
            x = (c_2 - c_1) / (d_1 - d_2)
            if x > x_min and x < x_max:
                intersections = [x]
    elif a_1 == 0 and a_2 != 0:
        if math.fabs(d_1) > eps:
            intersections = deg3_real_roots(-d_1, c_2 - c_1 - d_1 * b_2, 0, a_2)
            intersections = [z+b_2 for z in intersections if z+b_2 > x_min and z+b_2 < x_max]
        elif math.fabs(c_1 - c_2) < eps:
            return []
        else:
            x = b_2 + np.sqrt(a_2 / (c_2 - c_1))
            if x > x_min and x < x_max:
                intersections = [x]
    elif a_1 != 0 and a_2 == 0:
        if math.fabs(d_2) > eps:
            intersections = deg3_real_roots(-d_2, c_1 - c_2 - d_2 * b_1, 0, a_1)
            intersections = [z+b_1 for z in intersections if z+b_1 > x_min and z+b_1 < x_max]
        elif math.fabs(c_1 - c_2) < eps:
            return []
        else:
            x = b_1 + np.sqrt(a_1 / (c_1 - c_2))
            if x > x_min and x < x_max:
                intersections = [x]
    else:
        # FIXME two hyperbolic functions generaly don't intersect
        # but we need a better test
        return []
    return intersections

# Takes a list of intervals of the form (x_min, x_max, ...) and a key function
# that maps an interval and an x value to its y value.
# Returns a list (x_min, x_max, index) where index refers to the input interval
def lower_envelop(intervals, key):
    assert(np.all([float('inf') > key(interval, interval[0]) for interval in intervals]))
    assert(np.all([math.fabs(interval[0] - interval[1]) > eps for interval in intervals]))

    start_points = [(interval[0], i) for (i, interval) in enumerate(intervals)]
    end_points = [(interval[1], i) for (i, interval) in enumerate(intervals)]
    points = sorted(start_points + end_points)
    active_intervals = []
    minimum = []
    while len(points) > 0:
        current_x = points[0][0]
        # process all events
        while len(points) > 0 and points[0][0] <= current_x + eps:
            x, index = points.pop(0)
            if index >= 0:
                interval = intervals[index]
                if interval[0] + eps >= current_x:
                    active_intervals.append(index)
                else:
                    assert(interval[1] + eps >= current_x)
                    active_intervals.remove(index)
        if len(active_intervals) > 0:
            next_x = points[0][0]
            epsilon_key = lambda a: (eps_round(a[0]), eps_round(a[1]))
            sorted_intervals = sorted([(key(intervals[index], current_x), key(intervals[index], next_x), index) for index in active_intervals], key=epsilon_key)
            current_y = sorted_intervals[0][0]
            minimum.append((current_x, sorted_intervals[0][2]))
            for (y_1, y_1_next, index_1), (y_2, y_2_next, index_2) in zip(sorted_intervals[:-1], sorted_intervals[1:]):
                # FIXME this need to be replaced by a real intersection test
                #if y_1 < y_2 and y_1_next > y_2_next:
                x_1_min, x_1_max, _, f_1 = intervals[index_1]
                x_2_min, x_2_max, _, f_2 = intervals[index_2]
                x_min, x_max = intersect((x_1_min, x_1_max), (x_2_min, x_2_max))
                x_min = max(current_x, x_min)
                if x_min < x_max:
                    intersections = intersect_functions(f_1, f_2, (x_min, x_max))
                    points += [(x, -1) for x in intersections]
                    points = sorted(points)

    xs = [x for x, _ in minimum] + [float('inf')]
    idxs = [index for _, index in minimum]

    result = []
    for x_min, x_max, idx in zip(xs[:-1], xs[1:], idxs):
        result.append((x_min, x_max, idx))
    return result
