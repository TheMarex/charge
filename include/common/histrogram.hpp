#ifndef CHARGE_COMMON_HISTOGRAM_HPP
#define CHARGE_COMMON_HISTOGRAM_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <ostream>

namespace charge::common {

namespace detail {
struct my_numpunct : std::numpunct<char> {
    std::string do_grouping() const { return "\03"; }
};

struct number_printer {
    friend auto &operator<<(std::ostream &ss, const number_printer& printer) {
        auto old_loc = ss.getloc();
        std::locale loc(ss.getloc(), new my_numpunct);
        ss.imbue(loc);
        ss << printer.number;
        ss.imbue(old_loc);
        return ss;
    }
    std::size_t number;
};

auto formated_number(std::size_t number) { return number_printer{number}; }

std::size_t count_chars(std::size_t number) {
    std::size_t degits = std::max(0.0, std::log10(number))+1;
    std::size_t spaces = std::max(0.0, std::ceil(degits/3.0) - 1);
    return degits + spaces;
}
}

template <typename T, std::size_t NUM_BINS = 10>
std::string measurements_to_historgram(const std::vector<T> &measurements) {
    if (measurements.size() == 0)
        return "No histogram data.\n";

    auto sorted_measurements = measurements;
    std::sort(sorted_measurements.begin(), sorted_measurements.end());
    auto p95 = sorted_measurements[sorted_measurements.size() * 0.95];
    auto p50 = sorted_measurements[sorted_measurements.size() * 0.5];
    auto p05 = sorted_measurements[sorted_measurements.size() * 0.05];
    auto min = sorted_measurements.front();
    auto max = sorted_measurements.back();
    auto span = p95 - p05;
    auto bin_width = span / NUM_BINS;

    if (bin_width == 0)
        return "No histogram data.\n";

    std::array<std::size_t, NUM_BINS> bins;
    std::fill(bins.begin(), bins.end(), 0);
    for (const auto &m : measurements) {
        std::size_t index =
            std::min<std::size_t>(NUM_BINS - 1, std::max<int>(0, std::ceil((m - min) / bin_width)));
        bins[index]++;
    }

    auto max_count =
        std::accumulate(bins.begin(), bins.end(), 0, [](const auto lhs, const auto rhs) {
            return std::max<std::size_t>(lhs, rhs);
        });

    std::stringstream ss;

    auto max_number_chars = 0;
    auto lower = min;
    for (auto row = 0; row < NUM_BINS; ++row) {
        auto upper = p05 + bin_width * (row + 1);
        if (row == NUM_BINS - 1)
            upper = max;
        max_number_chars = std::max<std::size_t>(max_number_chars, detail::count_chars(lower) + detail::count_chars(upper) + 6);
        lower = upper;
    }
    max_number_chars += 5;

    std::size_t NUM_COLUMNS = 80-max_number_chars;
    lower = min;
    for (auto row = 0; row < NUM_BINS; ++row) {
        auto upper = p05 + bin_width * (row + 1);
        if (row == NUM_BINS - 1)
            upper = max;
        ss << "[" << detail::formated_number(lower) << " - " << detail::formated_number(upper)
           << ") ";
        auto number_chars = detail::count_chars(lower) + detail::count_chars(upper) + 6;
        for (auto c = number_chars; c < max_number_chars; ++c)
            ss << " ";
        lower = upper;

        auto num_bars = bins[row] / (double)max_count * NUM_COLUMNS;
        for (auto bar = 0; bar < num_bars; ++bar)
            ss << "#";
        ss << "(" << bins[row] << ")" << std::endl;
    }

    ss << sorted_measurements.size() << " measurements with: p95 " << detail::formated_number(p95)
       << ", p50 " << detail::formated_number(p50) << ", p05 " << detail::formated_number(p05)
       << " (" << detail::formated_number(min) << "," << detail::formated_number(max) << ")"
       << std::endl;
    return ss.str();
}
}

#endif
