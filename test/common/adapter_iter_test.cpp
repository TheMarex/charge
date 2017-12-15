#include "common/adapter_iter.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Read transformed", "[adpater iter]")
{
    struct Wrapped
    {
        double value;
    };
    std::vector<Wrapped> values = {{0.0}, {2.0}, {6.0}, {7.0}, {10.0}};

    const auto transform = [](Wrapped& wrapped) -> double& {
        return wrapped.value;
    };

    std::vector<double> result;
    auto begin = make_adapter_iter(values.begin(), transform);
    auto end = make_adapter_iter(values.end(), transform);
    std::copy(begin, end, std::back_inserter(result));

    std::vector<double> reference {0, 2, 6, 7, 10};
    REQUIRE(result == reference);
}

TEST_CASE("Reverse read transformed", "[adpater iter]")
{
    struct Wrapped
    {
        double value;
    };
    std::vector<Wrapped> values = {{0.0}, {2.0}, {6.0}, {7.0}, {10.0}};

    const auto transform = [](Wrapped& wrapped) -> double& {
        return wrapped.value;
    };

    std::vector<double> result;
    auto begin = make_adapter_iter(values.begin(), transform);
    auto end = make_adapter_iter(values.end(), transform);
    auto rbegin = std::make_reverse_iterator(end);
    auto rend = std::make_reverse_iterator(begin);
    std::copy(rbegin, rend, std::back_inserter(result));

    std::vector<double> reference {10, 7, 6, 2, 0};
    REQUIRE(result == reference);
}

TEST_CASE("Write transformed", "[adpater iter]")
{
    struct Wrapped
    {
        double value;
        bool operator==(const Wrapped& other) const { return value == other.value; }
    };
    std::vector<Wrapped> values = {{0.0}, {2.0}, {6.0}, {7.0}, {10.0}};

    const auto transform = [](Wrapped& wrapped) -> double& {
        return wrapped.value;
    };

    std::vector<double> result;
    auto begin = make_adapter_iter(values.begin(), transform);
    auto end = make_adapter_iter(values.end(), transform);
    std::vector<double> input {10, 5, 9, 11, 1};
    std::copy(input.begin(), input.end(), begin);

    std::vector<Wrapped> reference = {{10.0}, {5.0}, {9.0}, {11.0}, {1.0}};
    REQUIRE(values == reference);
}

