#ifndef CHARGE_COMMON_ROOTS_HPP
#define CHARGE_COMMON_ROOTS_HPP

#include <cassert>
#include <cmath>
#include <complex>
#include <optional>
#include <tuple>

namespace charge::common {

namespace detail {

inline void unique(std::optional<double> &z_0, std::optional<double> &z_1) {
    if (z_0 && z_1 && std::fabs(*z_0 - *z_1) < 1e-5) {
        z_1 = {};
    } else if (!z_0) {
        std::swap(z_0, z_1);
    }
}

inline void unique(std::optional<double> &z_0, std::optional<double> &z_1,
                   std::optional<double> &z_2) {
    unique(z_0, z_2);
    unique(z_0, z_1);
    unique(z_1, z_2);
}

inline void unique(std::optional<double> &z_0, std::optional<double> &z_1,
                   std::optional<double> &z_2, std::optional<double> &z_3) {
    unique(z_0, z_3);
    unique(z_0, z_2);
    unique(z_0, z_1);
    unique(z_1, z_2, z_3);
}
}

inline std::optional<double> real_roots(double a, double b) noexcept {
    if (a == 0) {
        return {};
    }

    return -b / a;
}

inline std::tuple<std::optional<double>, std::optional<double>> real_roots(double a, double b,
                                                                           double c) noexcept {
    if (a == 0) {
        return {real_roots(b, c), {}};
    }

    assert(a != 0);

    auto p = b / a;
    auto q = c / a;

    auto N = p * p / 4 - q;
    if (N < 0) {
        return {};
    } else if (N > 0) {

        auto x_0 = -p / 2 + std::sqrt(N);
        auto x_1 = -p / 2 - std::sqrt(N);

        return {{x_0}, {x_1}};
    } else {
        return {{-p / 2}, {-p / 2}};
    }
}

inline std::tuple<std::optional<double>, std::optional<double>, std::optional<double>>
real_roots(double a, double b, double c, double d) noexcept {
    if (a == 0) {
        auto[x_0, x_1] = real_roots(b, c, d);
        return {x_0, x_1, {}};
    }

    assert(a != 0);
    assert(std::isfinite(a));
    assert(std::isfinite(b));
    assert(std::isfinite(c));
    assert(std::isfinite(d));

    const double p = (3 * a * c - b * b) / (3 * a * a);
    const double q = (2 * b * b * b - 9 * a * b * c + 27 * a * a * d) / (27 * a * a * a);
    const auto x = [=](double t) { return t - b / (3 * a); };
    const double s = 4 * p * p * p + 27 * q * q;

    if (s > 0) {
        if (p > 0) {
            auto t_0 = -2 * std::sqrt(p / 3) *
                       std::sinh(1 / 3. * std::asinh(3 * q / (2 * p) * std::sqrt(3 / p)));
            return {x(t_0), {}, {}};
        } else if (p < 0) {
            auto t_0 =
                -2 * std::copysign(1.0, q) * std::sqrt(-p / 3) *
                std::cosh(1 / 3. * std::acosh(-3 * std::abs(q) / (2 * p) * std::sqrt(-3 / p)));
            return {x(t_0), {}, {}};
        } else {
            return {{}, {}, {}};
        }
    } else {
        assert(s <= 0);
        if (p < 0) {
            auto t_k = [&](auto k) -> double {
                return 2 * std::sqrt(-p / 3) *
                       std::cos(1 / 3. * std::acos(3 * q / (2 * p) * std::sqrt(-3 / p)) -
                                2 * k * M_PI / 3);
            };
            return {x(t_k(0)), x(t_k(1)), x(t_k(2))};
        } else if (p > 0) {
            return {};
        } else {
            return {x(0), {}, {}};
        }
    }
}

inline std::tuple<std::optional<double>, std::optional<double>, std::optional<double>,
                  std::optional<double>>
real_roots(double a_0, double b_0, double c_0, double d_0, double e_0) noexcept {
    if (a_0 == 0) {
        auto[x_0, x_1, x_2] = real_roots(b_0, c_0, d_0, e_0);
        return {x_0, x_1, x_2, {}};
    }

    assert(a_0 != 0);
    assert(std::isfinite(a_0));
    assert(std::isfinite(b_0));
    assert(std::isfinite(c_0));
    assert(std::isfinite(d_0));
    assert(std::isfinite(e_0));

    const auto b = b_0 / a_0;
    const auto c = c_0 / a_0;
    const auto d = d_0 / a_0;
    const auto e = e_0 / a_0;

    // this would cause underflows
    if (std::fabs(b * b) < 1e-10 && std::fabs(d * d) < 1e-10) {
        // x^4 + cx^2 + e = 0
        // y^2 + cy + e = 0
        auto[y_0, y_1] = real_roots(1, c, e);
        assert(y_0);
        assert(y_1);

        const auto x_0 = std::sqrt(*y_0);
        const auto x_1 = -std::sqrt(*y_0);
        const auto x_2 = std::sqrt(*y_1);
        const auto x_3 = -std::sqrt(*y_1);

        return {x_0, x_1, x_2, x_3};
    }

    const auto p = (8 * c - 3 * b * b) / 8.0;
    const auto q = (b * b * b - 4 * b * c + 8 * d) / 8.0;
    const auto r = (-3 * b * b * b * b + 256 * e - 64 * b * d + 16 * b * b * c) / 256.0;

    const auto y_to_x = -b / 4;

    // y^4 + p*y^2 + r = 0
    // z = y^2
    // z^2 + p*z + r = 0
    if (std::fabs(q * q) < 1e-10) {
        auto[z_0, z_1] = real_roots(1, p, r);
        if (!z_0)
        {
            assert(!z_1);
            return {};
        }

        auto y_0 = std::sqrt(*z_0);
        auto y_1 = -std::sqrt(*z_0);
        auto y_2 = std::sqrt(*z_1);
        auto y_3 = -std::sqrt(*z_1);

        auto x_0 = y_0 + y_to_x;
        auto x_1 = y_1 + y_to_x;
        auto x_2 = y_2 + y_to_x;
        auto x_3 = y_3 + y_to_x;

        if (z_0 < 0 && z_1 < 0)
            return {};
        else if (z_1 < 0)
            return {x_0, x_1, {}, {}};
        else if (z_0 < 0)
            return {x_2, x_3, {}, {}};

        return {x_0, x_1, x_2, x_3};
    }

    const auto a_1 = 8;
    const auto b_1 = 8 * p;
    const auto c_1 = 2 * p * p - 8 * r;
    const auto d_1 = -q * q;
    auto[m_0, m_1, m_2] = real_roots(a_1, b_1, c_1, d_1);
    if (!m_0) {
        return {};
    }
    auto m = *m_0;
    assert(m != 0);

    auto k_0 = std::sqrt(2 * m);
    auto k_1 = -std::sqrt(2 * m);
    auto n_0 = -2 * (p + m + q / k_0);
    auto n_1 = -2 * (p + m + q / k_1);

    if (n_0 >= 0) {
        auto y_0 = (k_0 + std::sqrt(n_0)) / 2.0;
        auto y_1 = (k_0 - std::sqrt(n_0)) / 2.0;
        auto x_0 = y_0 + y_to_x;
        auto x_1 = y_1 + y_to_x;

        if (n_1 >= 0) {
            auto y_2 = (k_1 + std::sqrt(n_1)) / 2.0;
            auto y_3 = (k_1 - std::sqrt(n_1)) / 2.0;
            auto x_2 = y_2 + y_to_x;
            auto x_3 = y_3 + y_to_x;

            return {x_0, x_1, x_2, x_3};
        }

        return {x_0, x_1, {}, {}};
    } else if (n_1 >= 0) {
        auto y_0 = (k_1 + std::sqrt(n_1)) / 2.0;
        auto y_1 = (k_1 - std::sqrt(n_1)) / 2.0;

        auto x_0 = y_0 + y_to_x;
        auto x_1 = y_1 + y_to_x;

        return {x_0, x_1, {}, {}};
    }

    return {};
}

/*

// From http://www.ijpam.eu/contents/2011-71-2/7/7.pdf
inline std::tuple<std::optional<double>, std::optional<double>, std::optional<double>,
                  std::optional<double>>
real_roots(double a_0, double b_0, double c_0, double d_0, double e_0) noexcept {
    if (a_0 == 0) {
        auto[x_0, x_1, x_2] = real_roots(b_0, c_0, d_0, e_0);
        return {x_0, x_1, x_2, {}};
    }

    assert(a_0 != 0);
    assert(std::isfinite(a_0));
    assert(std::isfinite(b_0));
    assert(std::isfinite(c_0));
    assert(std::isfinite(d_0));
    assert(std::isfinite(e_0));

    const auto a = b_0 / a_0;
    const auto b = c_0 / a_0;
    const auto c = d_0 / a_0;
    const auto d = e_0 / a_0;

    // depressed solution
    auto[y_0, y_1, y_2] = real_roots(1, -b, a * c - 4 * d, -a * a * d - c * c + 4 * b * d);
    if (!y_0) {
        return {};
    }
    auto y = *y_0;

    const auto e_square = a * a / 4 - b + y;
    const auto f_square = y * y / 4 - d;

    if (e_square < 0) {
        return {};
    }
    if (f_square < 0) {
        return {};
    }

    const auto e = std::sqrt(e_square);
    const auto f = std::sqrt(f_square);

    auto G = a / 2 + e;
    auto g = a / 2 - e;
    auto H = y / 2 + f;
    auto h = y / 2 - f;

    assert((h*h - y*h + d) == 0);
    assert((H*H - y*H + d) == 0);

    assert((g*g - a*g + b - y) == 0);
    assert((G*G - a*G + b - y) == 0);

    assert((h + H) == y);
    assert((G + g) == a);
    assert((G * g  + h + H) == b);
    assert((G * h  + g * H) == c);
    assert((h * H) == d);

    auto N = G * G / 4 - H;
    auto n = g * g / 4 - h;

    if (N >= 0 && n >= 0) {
        auto x_0 = -G / 2 + std::sqrt(N);
        auto x_1 = -G / 2 - std::sqrt(N);
        auto x_2 = -g / 2 + std::sqrt(n);
        auto x_3 = -g / 2 - std::sqrt(n);

        return {x_0, x_1, x_2, x_3};
    } else if (N >= 0) {
        auto x_0 = -G / 2 + std::sqrt(N);
        auto x_1 = -G / 2 - std::sqrt(N);
        return {x_0, x_1, {}, {}};
    } else if (n >= 0) {
        auto x_0 = -g / 2 + std::sqrt(n);
        auto x_1 = -g / 2 - std::sqrt(n);
        return {x_0, x_1, {}, {}};
    } else {
        return {};
    }
}
*/

inline std::tuple<std::optional<double>, std::optional<double>, std::optional<double>,
                  std::optional<double>>
unique_real_roots(double a, double b, double c, double d, double e) noexcept {
    auto[z_0, z_1, z_2, z_3] = real_roots(a, b, c, d, e);
    detail::unique(z_0, z_1, z_2, z_3);
    return {z_0, z_1, z_2, z_3};
}

inline std::tuple<std::optional<double>, std::optional<double>, std::optional<double>>
unique_real_roots(double a, double b, double c, double d) noexcept {
    auto[z_0, z_1, z_2] = real_roots(a, b, c, d);
    detail::unique(z_0, z_1, z_2);
    return {z_0, z_1, z_2};
}

inline std::tuple<std::optional<double>, std::optional<double>>
unique_real_roots(double a, double b, double c) noexcept {
    auto[z_0, z_1] = real_roots(a, b, c);
    detail::unique(z_0, z_1);
    return {z_0, z_1};
}
}

#endif
