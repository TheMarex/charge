#include "common/sink_iter.hpp"

#include <catch.hpp>

#include <functional>

using namespace charge;
using namespace charge::common;

TEST_CASE("Transform output iter - rvalue", "[sink iter]") {
    struct Wrapped {
        double value;
    };
    std::vector<Wrapped> values = {{0.0}, {2.0}, {6.0}, {7.0}, {10.0}};

    std::vector<double> result;
    std::copy(values.begin(), values.end(),
              make_sink_iter([&](const auto &w) { result.push_back(w.value); }));

    std::vector<double> reference{0, 2, 6, 7, 10};
    REQUIRE(result == reference);
}

TEST_CASE("Transform output iter - lvalue", "[sink iter]") {
    struct Wrapped {
        double value;
    };
    std::vector<Wrapped> values = {{0.0}, {2.0}, {6.0}, {7.0}, {10.0}};
    std::vector<double> result;
    const auto fn = [&](const auto &w) { result.push_back(w.value); };
    auto iter = make_sink_iter(std::ref(fn));
    auto other = make_sink_iter(std::ref(fn));
    iter = other;
    std::copy(values.begin(), values.end(), make_sink_iter(fn));

    std::vector<double> reference{0, 2, 6, 7, 10};
    REQUIRE(result == reference);
}
