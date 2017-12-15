import math
import numpy as np

from functions import *
from constants import eps

def __flip_d(intervals):
    new_intervals = []
    # fix delta functions d = x - d'
    for interval in intervals:
        x_min, x_max, d_rev, h = interval
        a_rev, b_rev = d_rev.params()
        d = LinearFunction(1 - a_rev, -b_rev)
        new_intervals.append((x_min, x_max, d, h))

    return new_intervals

def __link_charging(f, cf, M):
    cf_inv = invert_piecewise_linear(cf)
    candidates = []
    for x_min, x_max, sub_f in f.intervals():
        # only for non-linear functions the best
        # trade-off point is not on the boundary
        if len(sub_f.params()) == 4:
            a, b, c, _ = sub_f.params()
            for z_min, z_max, sub_cf in cf.intervals():
                a_i, b_i = sub_cf.params()
                assert(a_i >= 0)
                if a_i > eps:
                    x_i = b + np.cbrt(2*a / a_i)
                    if x_i > x_min and x_i < x_max:
                        z = cf_inv(M - sub_f(x_i))
                        if z >= z_min and z <= z_max:
                            candidates.append(x_i)
        candidates.append(x_min)

    intervals = []
    for x_min, x_max, sub_f in f.intervals():
        opt_d = LinearFunction(1, 0)
        intervals.append((x_min, x_max, opt_d, sub_f))

    print(intervals)

    for d in candidates:
        if M - f(d) <= eps:
            continue

        z_0 = cf_inv(M - f(d))
        clipped_cf = clip(cf, z_0)
        if clipped_cf:
            h = offset(multiply(shift(shift(clipped_cf, z_0), -d), -1), M)
            opt_d = LinearFunction(0, d)
            for z_min, z_max, sub_h in h.intervals():
                z_min = max(d, z_min)
                if z_min < z_max:
                    intervals.append((z_min, z_max, opt_d, sub_h))

    return intervals

def __combine_consumption(f, g, d):
    """Returns hypberbolic function that is the result of f(d) + g(x - d)
    """
    a_1, b_1, c_1, _ = f.params()
    a_2, b_2, c_2, _ = g.params()

    d_a, d_b = d.params()
    if math.fabs(d_a) < eps:
        a_3 = a_2
        b_3 = b_2 + d_b
        c_3 = c_2 + f(d_b)
    elif math.fabs(d_a-1) < eps:
        # f(d) + g(x - d) = f(x + d_b) + g(x - x - d_b) = f(x + d_b) + g(-d_b)
        return __combine_consumption(g, f, LinearFunction(0, -d_b))
    else:
        #assert(math.fabs(d_a - (1 + np.cbrt(a_1 / a_2))) < eps)
        #assert(math.fabs(d_b - (-b_1 + b_2 * np.cbrt(a_1 / a_2))) < eps)
        a_3 = a_1 + a_2 + 3 * (np.cbrt(a_1*a_1*a_2) + np.cbrt(a_1*a_2*a_2))
        b_3 = b_1 + b_2
        c_3 = c_1 + c_2
    if math.fabs(a_3) > eps:
        return HypLinFunction(a_3, b_3, c_3, 0)
    else:
        return LinearFunction(0, c_3)

def __link_consumption(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g):
    x_max = x_f_max + x_g_max
    x_min = x_f_min + x_g_min
    intervals = []
    if x_max < float('inf'):
        y_min = sub_f(x_f_max) + sub_g(x_g_max)
        intervals.append((x_max, float('inf'), LinearFunction(0, x_f_max), LinearFunction(0, y_min)))

    if x_min < x_max:
        if len(sub_f.params()) == 2 and len(sub_g.params()) == 2:
            intervals += __link_consumption_lin_lin(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g)
        elif len(sub_f.params()) == 4 and len(sub_g.params()) == 2:
            intervals += __link_consumption_hyp_lin(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g)
        elif len(sub_f.params()) == 2 and len(sub_g.params()) == 4:
            intervals += __link_consumption_lin_hyp(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g)
        elif len(sub_f.params()) == 4 and len(sub_g.params()) == 4:
            intervals += __link_consumption_hyp_hyp(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g)
        else:
            raise Exception("Unexpected function type")
    return intervals

def __link_consumption_lin_lin(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g):
    a_1, b_1 = sub_f.params()
    a_2, b_2 = sub_g.params()

    x_max = x_f_max + x_g_max
    x_min = x_f_min + x_g_min
    y_min = sub_f(x_f_max) + sub_g(x_g_max)

    intervals = []

    # depending on which has a steper decline we first drive slower on the first segement
    # then on the second segment
    if a_1 >= a_2:
        # first g
        if eps + x_min < x_g_max + x_f_min:
            intervals.append((x_min, x_g_max + x_f_min, LinearFunction(0, x_f_min), LinearFunction(a_2, b_2 - a_2 * x_f_min + sub_f(x_f_min))))
        # then f
        if eps + x_g_max + x_f_min < x_max:
            intervals.append((x_g_max + x_f_min, x_max, LinearFunction(1, -x_g_max), LinearFunction(a_1, b_1 - a_1 * x_g_max + sub_g(x_g_max))))
    else:
        # first f
        if eps + x_min < x_f_max + x_g_min:
            intervals.append((x_min, x_f_max + x_g_min, LinearFunction(1, -x_g_min), LinearFunction(a_1, b_1 - a_1 * x_g_min + sub_g(x_g_min))))
        # then g
        if eps + x_f_max + x_g_min < x_max:
            intervals.append((x_f_max + x_g_min, x_max, LinearFunction(0, x_f_max), LinearFunction(a_2, b_2 - a_2 * x_f_max + sub_f(x_f_max))))

    return intervals


def __link_consumption_lin_hyp(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g):
    return __flip_d(__link_consumption_hyp_lin(x_g_min, x_g_max, x_f_min, x_f_max, sub_g, sub_f))

def __link_consumption_hyp_lin(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g):
    a_1, b_1, c_1, _ = sub_f.params()
    a_2, b_2 = sub_g.params()

    x_max = x_f_max + x_g_max
    x_min = x_f_min + x_g_min
    y_min = sub_f(x_f_max) + sub_g(x_g_max)

    intervals = []
    if math.fabs(a_2) > eps:
        d_star = b_1 + np.cbrt(-2 * a_1 / a_2)
    else:
        d_star = float('inf')

    # implies f'(x) > g'(x) on [x_min, x_max]
    # so we first drive slower on g then on f
    if d_star < x_f_min:
        if eps + x_min < x_g_max + x_f_min:
            intervals.append((x_min, x_g_max + x_f_min, LinearFunction(0, x_f_min), LinearFunction(a_2, b_2 - a_2 * x_f_min + sub_f(x_f_min))))
        if eps + x_g_max + x_f_min < x_max:
            intervals.append((x_g_max + x_f_min, x_max, LinearFunction(1, -x_g_max), HypLinFunction(a_1, b_1 + x_g_max, c_1 + sub_g(x_g_max), 0)))
    # implies f'(x) < g'(x) on [x_min, x_max]
    # so we first drive slower on f then on g
    elif d_star > x_f_max:
        if eps + x_min < x_f_max + x_g_min:
            intervals.append((x_min, x_f_max + x_g_min, LinearFunction(1, -x_g_min), HypLinFunction(a_1, b_1 + x_g_min, c_1 + sub_g(x_g_min), 0)))
        if eps + x_f_max + x_g_min < x_max:
            intervals.append((x_f_max + x_g_min, x_max, LinearFunction(0, x_f_max), LinearFunction(a_2, b_2 - a_2 * x_f_max + sub_f(x_f_max))))
    # implies f'(x) < g'(x) on [x_min, d_star] and f'(x) > g'(x) on [d_star, x_max]
    # so we first drive slower on f then on g and then on f again
    else:
        if eps + x_min < d_star+x_g_min:
            intervals.append((x_min, d_star+x_g_min, LinearFunction(1, -x_g_min), HypLinFunction(a_1, b_1 + x_g_min, c_1 + sub_g(x_g_min), 0)))
        if eps + d_star+x_g_min < d_star + x_g_max:
            intervals.append((d_star+x_g_min, d_star + x_g_max, LinearFunction(0, d_star), LinearFunction(a_2, b_2 - a_2 * d_star + sub_f(d_star))))
        if eps + d_star + x_g_max < x_max:
            intervals.append((d_star + x_g_max, x_max, LinearFunction(1, -x_g_max), HypLinFunction(a_1, b_1 + x_g_max, c_1 + sub_g(x_g_max), 0)))

    return intervals

def __link_consumption_hyp_hyp(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g):
    a_1, b_1, c_1, _ = sub_f.params()
    a_2, b_2, c_2, _ = sub_g.params()
    assert(a_1 > 0)
    assert(a_2 > 0)

    f_min_deriv = -2 * a_1 / (x_f_min - b_1)**3
    g_min_deriv = -2 * a_2 / (x_g_min - b_2)**3

    if f_min_deriv > g_min_deriv:
        intervals = __link_consumption_hyp_hyp(x_g_min, x_g_max, x_f_min, x_f_max, sub_g, sub_f)
        return __flip_d(intervals)

    f_max_deriv = -2 * a_1 / (x_f_max - b_1)**3
    g_max_deriv = -2 * a_2 / (x_g_max - b_2)**3

    assert(f_min_deriv <= g_min_deriv)
    assert(f_min_deriv <= f_max_deriv)
    assert(g_min_deriv <= g_max_deriv)
    assert(f_min_deriv < 0)
    assert(f_max_deriv < 0)
    assert(g_min_deriv < 0)
    assert(g_max_deriv < 0)

    x_max = x_f_max + x_g_max
    x_min = x_f_min + x_g_min
    y_min = sub_f(x_f_max) + sub_g(x_g_max)

    assert(a_1 > 0)
    assert(a_2 > 0)

    # linear coeeficients for d_star
    d_star_a = 1 / (1 + np.cbrt(a_2 / a_1))
    d_star_b = (-b_2 + b_1 * np.cbrt(a_2 / a_1)) * d_star_a
    d_star = LinearFunction(d_star_a, d_star_b)

    # because we enforce f_min_deriv < g_min_deriv these values are never needed
    # d_start(x) > x_f_min
    #x_f_min_star = x_f_min + b_2 + np.cbrt(a_2 / a_1) * (x_f_min - b_1)

    # d_start(x) < x_f_max
    x_f_max_star = x_f_max + b_2 + np.cbrt(a_2 / a_1) * (x_f_max - b_1)

    # d_star(x) > x - x_g_min
    x_g_min_star = x_g_min + b_1 + np.cbrt(a_1 / a_2) * (x_g_min - b_2)
    # d_star(x) < x - x_g_max
    x_g_max_star = x_g_max + b_1 + np.cbrt(a_1 / a_2) * (x_g_max - b_2)

    intervals = []

    if g_min_deriv <= f_max_deriv and f_max_deriv < g_max_deriv:
        if eps + x_min < x_g_min_star:
            # spend more time on f
            d_first = LinearFunction(1, -x_g_min)
            intervals.append((x_min, x_g_min_star, d_first, __combine_consumption(sub_f, sub_g, d_first)))
        if eps + x_g_min_star < x_f_max_star:
            # spend more time on f and g
            d_middle = d_star
            intervals.append((x_g_min_star, x_f_max_star, d_middle, __combine_consumption(sub_f, sub_g, d_middle)))
        if eps + x_f_max_star < x_max:
            # spend more time on f
            d_last = LinearFunction(0, x_f_max)
            intervals.append((x_f_max_star, x_max, d_last, __combine_consumption(sub_f, sub_g, d_last)))
    elif f_max_deriv <= g_min_deriv:
        if eps + x_min < x_f_max + x_g_min:
            # spend more time on f
            d_first = LinearFunction(1, -x_g_min)
            intervals.append((x_min, x_f_max + x_g_min, d_first, __combine_consumption(sub_f, sub_g, d_first)))
        if eps + x_f_max + x_g_min < x_max:
            # spend more time on g
            d_last = LinearFunction(0, x_f_max)
            intervals.append((x_f_max + x_g_min, x_max, d_last, __combine_consumption(sub_f, sub_g, d_last)))
    elif g_max_deriv <= f_max_deriv:
        if eps + x_min < x_g_min_star:
            # first spend time on f
            d_first = LinearFunction(1, -x_g_min)
            intervals.append((x_min, x_g_min_star, d_first, __combine_consumption(sub_f, sub_g, d_first)))
        if eps + x_g_min_star < x_g_max_star:
            # then spend time on both
            d_middle = d_star
            intervals.append((x_g_min_star, x_g_max_star, d_middle, __combine_consumption(sub_f, sub_g, d_middle)))
        if eps + x_g_max_star < x_max:
            # then increase time spend on f
            d_last = LinearFunction(1, -x_g_max)
            intervals.append((x_g_max_star, x_max, d_last, __combine_consumption(sub_f, sub_g, d_last)))
    else:
        raise Exception("Unhandled case")

    return intervals

def __intervals_to_piecewise(intervals):
    key = lambda interval, x: interval[3](x)
    # filter infinite segments
    intervals = [interval for interval in intervals if key(interval, interval[0]) < float('inf')]
    envelop = lower_envelop(intervals, key)
    hs = []
    opt_ds = []
    xs = []
    for (x_min, x_max, index) in envelop:
        hs.append(intervals[index][3])
        opt_ds.append(intervals[index][2])
        xs.append(x_min)
    if len(xs) == 0 or xs[0] > 0:
        xs = [0] + xs
        hs = [LinearFunction(0, float('inf'))] + hs
        opt_ds = [LinearFunction(0, float('nan'))] + opt_ds
    return (PiecewiseFunction(xs, opt_ds), PiecewiseFunction(xs, hs))

def link_consumption(f, g):
    intervals = []
    for x_f_min, x_f_max, sub_f in f.intervals():
        if sub_f(x_f_min) < float('inf') and sub_f(x_f_max) < float('inf'):
            for x_g_min, x_g_max, sub_g in g.intervals():
                if sub_g(x_g_min) < float('inf') and sub_g(x_g_max) < float('inf'):
                    new_functions = __link_consumption(x_f_min, x_f_max, x_g_min, x_g_max, sub_f, sub_g)
                    intervals += new_functions
    return __intervals_to_piecewise(intervals)

def link_charging(f, cf, M):
    intervals = __link_charging(f, cf, M)
    return __intervals_to_piecewise(intervals)

