#include "logic.h"

#include <assert.h>
#include <math.h>

static void TestApproach(void)
{
    assert(fabsf(LogicApproach(0.0f, 10.0f, 2.5f) - 2.5f) < 0.0001f);
    assert(fabsf(LogicApproach(9.0f, 10.0f, 2.5f) - 10.0f) < 0.0001f);
    assert(fabsf(LogicApproach(4.0f, -2.0f, 3.0f) - 1.0f) < 0.0001f);
}

static void TestRing(void)
{
    LogicVec2 bullets[4];
    int count = LogicBuildRing(bullets, 4, 4, 0.0f, 10.0f);

    assert(count == 4);
    assert(fabsf(bullets[0].x - 10.0f) < 0.0001f);
    assert(fabsf(bullets[0].y) < 0.0001f);
    assert(fabsf(bullets[1].x) < 0.0001f);
    assert(fabsf(bullets[1].y - 10.0f) < 0.0001f);
}

static void TestFanToward(void)
{
    LogicVec2 bullets[3];
    int count = LogicBuildFanToward(bullets, 3, 3, (LogicVec2){0.0f, 0.0f},
                                    (LogicVec2){0.0f, 10.0f}, 1.0f, 5.0f);

    assert(count == 3);
    assert(bullets[1].y > 4.9f);
    assert(fabsf(bullets[1].x) < 0.0001f);
    assert(bullets[0].x > bullets[2].x);
}

static void TestCircleOverlap(void)
{
    assert(LogicCircleOverlap((LogicVec2){0.0f, 0.0f}, 4.0f, (LogicVec2){6.0f, 0.0f}, 2.5f));
    assert(!LogicCircleOverlap((LogicVec2){0.0f, 0.0f}, 3.0f, (LogicVec2){7.5f, 0.0f}, 2.0f));
}

int main(void)
{
    TestApproach();
    TestRing();
    TestFanToward();
    TestCircleOverlap();
    return 0;
}
