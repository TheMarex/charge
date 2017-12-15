#include "common/lazy_clear_vector.hpp"
#include "common/irange.hpp"

#include "catch.hpp"

#include <vector>

using namespace charge;
using namespace charge::common;

TEST_CASE("Simple lazy clear vector test", "[LazyClearVector]")
{
    LazyClearVector<unsigned> test_vector_0(3, 0);

    REQUIRE(test_vector_0[0] == 0);
    REQUIRE(test_vector_0[1] == 0);
    REQUIRE(test_vector_0[2] == 0);

    test_vector_0[0] = 1;
    REQUIRE(test_vector_0[0] == 1);
    REQUIRE(test_vector_0[1] == 0);
    REQUIRE(test_vector_0[2] == 0);
    test_vector_0.clear();

    test_vector_0[1] = 2;
    REQUIRE(test_vector_0[0] == 0);
    REQUIRE(test_vector_0[1] == 2);
    REQUIRE(test_vector_0[2] == 0);
    test_vector_0.clear();

    test_vector_0[2] = 5;
    REQUIRE(test_vector_0[0] == 0);
    REQUIRE(test_vector_0[1] == 0);
    REQUIRE(test_vector_0[2] == 5);

    LazyClearVector<unsigned> test_vector_1(3, 5);

    REQUIRE(test_vector_1[0] == 5);
    REQUIRE(test_vector_1[1] == 5);
    REQUIRE(test_vector_1[2] == 5);

    test_vector_1[0] = 1;
    REQUIRE(test_vector_1[0] == 1);
    REQUIRE(test_vector_1[1] == 5);
    REQUIRE(test_vector_1[2] == 5);
    test_vector_1.clear();

    test_vector_1[1] = 2;
    REQUIRE(test_vector_1[0] == 5);
    REQUIRE(test_vector_1[1] == 2);
    REQUIRE(test_vector_1[2] == 5);
    test_vector_1.clear();

    test_vector_1[2] = 5;
    REQUIRE(test_vector_1[0] == 5);
    REQUIRE(test_vector_1[1] == 5);
    REQUIRE(test_vector_1[2] == 5);

    LazyClearVector<unsigned> test_vector_2(3, 5);
    test_vector_2[0] = 1;
    test_vector_2[1] = 2;
    test_vector_2[2] = 3;
    for (auto idx : irange(0, 256))
    {
        test_vector_2.clear();
    }
    REQUIRE(test_vector_2[0] == 5);
    REQUIRE(test_vector_2[1] == 5);
    REQUIRE(test_vector_2[2] == 5);
}

