import numpy as np

from functions import make_piecewise_linear, PiecewiseFunction, LinearFunction

def link_consumption(T, f, g, M):
    L = f(T)
    R = g(T)
    max_t_idx = np.iinfo(np.dtype('uint32')).max
    opt_H = np.full_like(L, float('inf'))
    opt_delta_idx = np.full_like(L, max_t_idx, dtype='uint32')
    for d_idx in range(len(L)):
        R_d = np.roll(R, d_idx)
        R_d[:d_idx] = float('inf')
        if L[d_idx] >= float('inf'):
            continue
        H_d = np.maximum(0, L[d_idx] + R_d)
        H_d[R_d >= float('inf')] = float('inf')
        index = opt_H > H_d
        opt_H[index] = H_d[index]
        opt_delta_idx[index] = d_idx
    opt_H[opt_H > M] = float('inf')
    opt_delta_idx[opt_H > M] = max_t_idx

    opt_delta = np.full_like(T, float('inf'), dtype='float')
    opt_delta[opt_delta_idx < max_t_idx] = T[opt_delta_idx[opt_delta_idx < max_t_idx]]

    d = PiecewiseFunction(T, np.concatenate((make_piecewise_linear(T, opt_delta), [LinearFunction(0, opt_delta[-1])])))
    h = PiecewiseFunction(T, np.concatenate((make_piecewise_linear(T, opt_H), [LinearFunction(0, opt_H[-1])])))

    return d, h

def link_charging(T, f, cf, M):
    L = f(T)
    CF = cf(T)
    max_t_idx = np.iinfo(np.dtype('uint32')).max
    opt_H = np.full_like(L, float('inf'))
    opt_delta_idx = np.full_like(L, max_t_idx, dtype='uint32')
    ts = []
    for d_idx in range(len(L)):
        if L[d_idx] >= float('inf'):
            continue
        y = M - L[d_idx]
        if y < 0:
            continue
        assert(y <= M)
        t_idx = np.argmax(CF > y)
        CF_y = np.roll(CF, -t_idx)
        CF_y[-t_idx:] = CF[-1]
        assert(len(CF_y) == len(L))
        CF_d = np.roll(CF_y, d_idx)
        CF_d[:d_idx] = -float('inf')
        H_d = np.maximum(0, M - CF_d)
        index = opt_H > H_d
        opt_H[index] = H_d[index]
        opt_delta_idx[index] = d_idx
    opt_H[opt_H > M] = float('inf')
    opt_delta_idx[opt_H > M] = max_t_idx

    print(list(ts))

    opt_delta = np.full_like(T, float('inf'), dtype='float')
    opt_delta[opt_delta_idx < max_t_idx] = T[opt_delta_idx[opt_delta_idx < max_t_idx]]

    d = PiecewiseFunction(T, np.concatenate((make_piecewise_linear(T, opt_delta), [LinearFunction(0, opt_delta[-1])])))
    h = PiecewiseFunction(T, np.concatenate((make_piecewise_linear(T, opt_H), [LinearFunction(0, opt_H[-1])])))
    return d, h
