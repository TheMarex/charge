import numpy as np

def deg3_real_roots(a, b, c, d):
    assert(np.isfinite(a))
    assert(np.isfinite(b))
    assert(np.isfinite(c))
    assert(np.isfinite(d))
    """Returns the real roots of the polynom a*z^3 + b*z^2 + c*z + d = 0.
    >>> deg3_real_roots(1, 0, -15, -4) # would include complex terms
    [4.0, -0.26794919243112153, -3.7320508075688785]

    >>> deg3_real_roots(1, -9, 27, -27) # 3
    [3.0]

    >>> deg3_real_roots(1, 3, 0, 0)
    [2.2204460492503131e-16, 2.2204460492503131e-16, -3.0]
    """
    p = (3*a*c - b*b) / (3 * a *a)
    q = (2*b*b*b - 9 * a * b * c + 27*a*a*d) / (27 * a * a * a)
    x = lambda t: t - b/(3*a)
    d = 4*p*p*p + 27*q*q
    # three real roots
    if d > 0:
        if p > 0:
            t_0 = -2 * np.sqrt(p / 3) * np.sinh(1/3. * np.arcsinh(3*q / (2*p) * np.sqrt(3 / p)))
            return [x(t_0)]
        elif p < 0:
            t_0 = -2 * np.sign(q) * np.sqrt(-p / 3) * np.cosh(1/3. * np.arccosh(-3 * np.abs(q) / (2*p) * np.sqrt(-3 / p)))
            return [x(t_0)]
        else:
            return []
    else:
        assert(d <= 0)
        if p < 0:
            t_k = lambda k: 2 * np.sqrt(-p / 3) * np.cos(1/3. * np.arccos(3*q / (2*p) * np.sqrt(-3 / p)) - 2 * k * np.pi / 3)
            return [x(t_k(0)), x(t_k(1)), x(t_k(2))]
        elif p > 0:
            raise Exception("Complex root found")
        else:
            return [x(0)]

def intersect(a, b):
    return (max(a[0], b[0]), min(a[1], b[1]))

def get_sorting(l):
    asc = True
    dsc = True
    for idx in range(len(l)-1):
        asc = asc and l[idx] <= l[idx+1]
        dsc = dsc and l[idx] >= l[idx+1]
    return asc, dsc

if __name__ == "__main__":
    import doctest
    doctest.testmod()
