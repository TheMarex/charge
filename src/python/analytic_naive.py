import math
import numpy as np

from .functions import *

def _combine(sub_f, sub_cf, sub_cf_inv, sub_d, M):
    a_d, b_d = sub_d.params()
    # sub_d is a constant function
    if math.fabs(a_d) < eps:
        a_i, b_i = sub_cf.params()
        w_d = b_d - sub_cf_inv(M - sub_f(b_d))
        a = -a_i
        b = M + a_i * w_d - b_i
        return LinearFunction(a, b), "1"
    # sub_d is a linear functions
    else:
        # there should only be the case of sub_d(x) = x
        assert(math.fabs(a_d - 1) < eps)
        assert(math.fabs(b_d) < eps)
        return sub_f, "2"

def _get_split_f_images(f, cf):
    cf_inv = invert_piecewise_linear(cf)
    cf_inv_domains = list(cf_inv.domains())
    f_soc_intervals = [(M - sub_f(x_min), M - sub_f(x_max), x_min, x_max, sub_f) for x_min, x_max, sub_f in f.intervals()]
    f_soc_intervals = [(min(soc_1, soc_2), max(soc_1, soc_2), x_min, x_max, sub_f) for soc_1, soc_2, x_min, x_max, sub_f in f_soc_intervals]
    cf_inv_soc_intervals = cf_inv.intervals()

    # w(d) := cf^-1(M - f(d))
    f_images = []
    for f_soc_min, f_soc_max, x_min, x_max, sub_f in f_soc_intervals:
        for cf_inv_soc_min, cf_inv_soc_max, sub_cf_inv in cf_inv_soc_intervals:
            soc_min, soc_max = intersect((f_soc_min, f_soc_max), (cf_inv_soc_min, cf_inv_soc_max))
            if soc_min <= soc_max:
                f_images.append((soc_min, soc_max, x_min, x_max, (sub_f, sub_cf_inv)))
    return f_images

def _get_w_domains(f, cf):
    f_images = _get_split_f_images(f, cf)
    w_domains = []
    for (soc_min, soc_max, f_x_min, f_x_max, (sub_f, sub_cf_inv)) in f_images:
        if len(sub_f.params()) == 2:
            d, c = sub_f.params()
            a = 0
            b = 0
        else:
            a, b, c, d = sub_f.params()
        # f is a constant function
        if math.fabs(a) < eps and math.fabs(d) < eps:
            x_min = f_x_min
            x_max = f_x_max
        else:
            inv_sub_f = sub_f.inverse(domain=[f_x_min, f_x_max])
            x_min = inv_sub_f(M - soc_min)
            x_max = inv_sub_f(M - soc_max)
        if x_min <= x_max:
            w_domains.append((x_min, x_max, (sub_f, sub_cf_inv)))
    return w_domains

def link_naive(f, cf, M):
    w_domains = _get_w_domains(f, cf)
    intervals = []
    for z_min, z_max, sub_cf in cf.intervals():
        a_i, b_i = sub_cf.params()
        opt_d = None
        case = None
        # Case 2, 3
        for d_min, d_max, (sub_f, sub_cf_inv) in w_domains:
            if len(sub_f.params()) == 4:
                a, b, c, d = sub_f.params()
            else:
                d, c = sub_f.params()
                a = 0
                b = 0
            a_j, b_j = sub_cf_inv.params()

            # Case 3
            if a_i > 0 and a_j > 0:
                case = "3"
                gamma_i = -1./a_j
                # case 3.2
                if math.fabs(d - a_j) > eps:
                    d_star = b + np.cbrt(2*a / (1./a_j + d))

                    if d_star >= d_min and d_star < d_max:
                        case = "3.2.1"
                        opt_d = d_star
                    else:
                        secant = (sub_f(d_max) - sub_f(d_min)) / (d_max - d_min)
                        # Case 3.2.2.1
                        if secant > gamma_i:
                            case = "3.2.2.1"
                            opt_d = d_min
                        else:
                            case = "3.2.2.2"
                            opt_d = d_max
                # case 3.1
                else:
                    case = "3.1"
                    opt_d = d_min
            # Case 2
            elif a_i > 0:
                case = "2"
                opt_d = d_min
            # Case 1
            else:
                if math.fabs(a_i) >= eps:
                    raise Exception("Assumed a_i = 0 != {}".format(a_i))

                case = "1"
                # FIXME this should be opt_d = x
                opt_d = d_min

            x_1 = z_min + opt_d - sub_cf_inv(M - sub_f(opt_d))
            x_2 = z_max + opt_d - sub_cf_inv(M - sub_f(opt_d))
            x_min = max(b, min(x_1, x_2))
            x_max = max(0, max(x_1, x_2))

            sub_x_min = float('nan')
            sub_x_max = float('nan')

            sub_f_inv = sub_f.inverse(domain=[d_min, d_max])
            assert(a_j >= 0)
            if a_j > eps:
                y_1 = (z_max - a_j * M - b_j) / -a_j
                y_2 = (z_min - a_j * M - b_j) / -a_j
                y_min, y_max = intersect((y_1, y_2), (sub_f(d_max), sub_f(d_min)))
                if y_min <= y_max:
                    # f is a constant function
                    if math.fabs(a) < eps and math.fabs(d) < eps:
                        sub_x_min = d_min
                        sub_x_max = d_max
                    else:
                        sub_x_min = sub_f_inv(y_max)
                        sub_x_max = sub_f_inv(y_min)
                    assert(sub_x_min >= d_min)
                    assert(sub_x_max <= d_max)
            else:
                if b_j > z_min and b_j < z_max:
                    sub_x_min = d_min
                    sub_x_max = d_max

            if math.fabs(x_min - x_max) > eps:
                if opt_d > x_min and opt_d < x_max:
                    if math.fabs(x_min - opt_d) > eps:
                        sub_d = LinearFunction(1., 0.)
                        sub_h, sub_case = _combine(sub_f, sub_cf, sub_cf_inv, sub_d, M)
                        if math.fabs(sub_x_min - sub_x_max) > eps:
                            intervals.append((sub_x_min, sub_x_max, sub_d, sub_h, case + "-" + sub_case))
                    if math.fabs(x_max - opt_d) > eps:
                        sub_d = LinearFunction(0., opt_d)
                        sub_h, sub_case = _combine(sub_f, sub_cf, sub_cf_inv, sub_d, M)
                        if math.fabs(sub_x_min - sub_x_max) > eps:
                            intervals.append((opt_d, x_max, sub_d, sub_h, case + "-" + sub_case))
                elif opt_d < x_max:
                    assert(opt_d < x_min)
                    sub_d = LinearFunction(0., opt_d)
                    sub_h, sub_case = _combine(sub_f, sub_cf, sub_cf_inv, sub_d, M)
                    intervals.append((x_min, x_max, sub_d, sub_h, case + "-" + sub_case))
                else:
                    assert(opt_d > x_max)
                    sub_d = LinearFunction(1., 0.)
                    sub_h, sub_case = _combine(sub_f, sub_cf, sub_cf_inv, sub_d, M)
                    if math.fabs(sub_x_min - sub_x_max) > eps:
                        intervals.append((sub_x_min, sub_x_max, sub_d, sub_h, case + "-" + sub_case))

    return intervals

def link_charging(f, cf, M):
    intervals = link_naive(f, cf, M)
    envelop = lower_envelop(intervals, lambda interval, x: interval[3](x))
    hs = []
    opt_ds = []
    xs = []
    for (x_min, x_max, index) in envelop:
        hs.append(interval[index][3])
        opt_ds.append(interval[index][2])
        xs.append(x_min)
    return (PiecewiseFunction(xs, opt_ds), PiecewiseFunction(xs, hs))

def link_consumption(f, g):
    pass
