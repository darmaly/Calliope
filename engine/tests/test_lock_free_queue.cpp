#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "calliope/lock_free_queue.h"

using namespace calliope;
using Catch::Matchers::WithinAbs;

TEST_CASE("LockFreeQueue starts empty", "[LockFreeQueue]")
{
    // AbstractFifo with capacity N has N-1 usable slots (one slot reserved)
    LockFreeQueue<int, 16> q;
    REQUIRE(q.getNumReady() == 0);
    REQUIRE(q.getFreeSpace() == 15);
}

TEST_CASE("LockFreeQueue push and pop single item", "[LockFreeQueue]")
{
    LockFreeQueue<int, 16> q;
    REQUIRE(q.push(42));
    REQUIRE(q.getNumReady() == 1);

    int item = 0;
    REQUIRE(q.pop(item));
    REQUIRE(item == 42);
    REQUIRE(q.getNumReady() == 0);
}

TEST_CASE("LockFreeQueue FIFO ordering", "[LockFreeQueue]")
{
    LockFreeQueue<int, 16> q;
    q.push(1);
    q.push(2);
    q.push(3);

    int item = 0;
    q.pop(item);
    REQUIRE(item == 1);
    q.pop(item);
    REQUIRE(item == 2);
    q.pop(item);
    REQUIRE(item == 3);
}

TEST_CASE("LockFreeQueue pop from empty returns false", "[LockFreeQueue]")
{
    LockFreeQueue<int, 16> q;
    int item = 0;
    REQUIRE_FALSE(q.pop(item));
}

TEST_CASE("LockFreeQueue push to full returns false", "[LockFreeQueue]")
{
    // AbstractFifo(4) gives 3 usable slots
    LockFreeQueue<int, 4> q;
    REQUIRE(q.push(1));
    REQUIRE(q.push(2));
    REQUIRE(q.push(3));
    REQUIRE_FALSE(q.push(4));
}

TEST_CASE("LockFreeQueue wraps around correctly", "[LockFreeQueue]")
{
    LockFreeQueue<int, 4> q;

    // Fill partially and drain
    q.push(10);
    q.push(20);
    q.push(30);

    int item = 0;
    q.pop(item);
    REQUIRE(item == 10);
    q.pop(item);
    REQUIRE(item == 20);
    q.pop(item);
    REQUIRE(item == 30);

    // Now push more (wraps around internal buffer)
    q.push(40);
    q.push(50);
    q.push(60);

    q.pop(item);
    REQUIRE(item == 40);
    q.pop(item);
    REQUIRE(item == 50);
    q.pop(item);
    REQUIRE(item == 60);
}

TEST_CASE("LockFreeQueue reset clears all items", "[LockFreeQueue]")
{
    LockFreeQueue<int, 16> q;
    for (int i = 0; i < 5; ++i)
        q.push(i);

    REQUIRE(q.getNumReady() == 5);

    q.reset();
    REQUIRE(q.getNumReady() == 0);
    REQUIRE(q.getFreeSpace() == 15);
}

TEST_CASE("LockFreeQueue works with struct types", "[LockFreeQueue]")
{
    struct MeterData {
        float left;
        float right;
    };

    LockFreeQueue<MeterData, 8> q;
    MeterData in{0.75f, 0.5f};
    REQUIRE(q.push(in));

    MeterData out{0.0f, 0.0f};
    REQUIRE(q.pop(out));
    REQUIRE_THAT(static_cast<double>(out.left), WithinAbs(0.75, 0.001));
    REQUIRE_THAT(static_cast<double>(out.right), WithinAbs(0.5, 0.001));
}
