#include "logic.h"

#include <math.h>
#include <stddef.h>

static const float LOGIC_EPSILON = 0.0001f;
static const float LOGIC_TAU = 6.28318530717958647692f;

float LogicClamp(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

float LogicApproach(float current, float target, float delta)
{
    if (current < target)
    {
        current += delta;
        if (current > target)
        {
            return target;
        }
        return current;
    }

    current -= delta;
    if (current < target)
    {
        return target;
    }

    return current;
}

float LogicLengthSquared(LogicVec2 value) { return value.x * value.x + value.y * value.y; }

float LogicLength(LogicVec2 value) { return sqrtf(LogicLengthSquared(value)); }

LogicVec2 LogicAdd(LogicVec2 left, LogicVec2 right)
{
    return (LogicVec2){
        .x = left.x + right.x,
        .y = left.y + right.y,
    };
}

LogicVec2 LogicSub(LogicVec2 left, LogicVec2 right)
{
    return (LogicVec2){
        .x = left.x - right.x,
        .y = left.y - right.y,
    };
}

LogicVec2 LogicScale(LogicVec2 value, float scalar)
{
    return (LogicVec2){
        .x = value.x * scalar,
        .y = value.y * scalar,
    };
}

LogicVec2 LogicNormalize(LogicVec2 value)
{
    float length = LogicLength(value);

    if (length <= LOGIC_EPSILON)
    {
        return (LogicVec2){0.0f, 0.0f};
    }

    return LogicScale(value, 1.0f / length);
}

LogicVec2 LogicPolar(float angle, float magnitude)
{
    return (LogicVec2){
        .x = cosf(angle) * magnitude,
        .y = sinf(angle) * magnitude,
    };
}

float LogicDistanceSquared(LogicVec2 left, LogicVec2 right)
{
    return LogicLengthSquared(LogicSub(left, right));
}

bool LogicCircleOverlap(LogicVec2 a, float a_radius, LogicVec2 b, float b_radius)
{
    float radius_sum = a_radius + b_radius;
    return LogicDistanceSquared(a, b) <= radius_sum * radius_sum;
}

int LogicBuildRing(LogicVec2 *out, int capacity, int count, float start_angle, float speed)
{
    int emitted = 0;

    if (out == NULL || capacity <= 0 || count <= 0)
    {
        return 0;
    }

    if (count > capacity)
    {
        count = capacity;
    }

    for (int i = 0; i < count; ++i)
    {
        float fraction = (float)i / (float)count;
        out[i] = LogicPolar(start_angle + LOGIC_TAU * fraction, speed);
        emitted++;
    }

    return emitted;
}

int LogicBuildFanToward(LogicVec2 *out, int capacity, int count, LogicVec2 origin, LogicVec2 target,
                        float spread_angle, float speed)
{
    float center_angle;
    int emitted = 0;

    if (out == NULL || capacity <= 0 || count <= 0)
    {
        return 0;
    }

    if (count > capacity)
    {
        count = capacity;
    }

    center_angle = atan2f(target.y - origin.y, target.x - origin.x);

    if (count == 1)
    {
        out[0] = LogicPolar(center_angle, speed);
        return 1;
    }

    for (int i = 0; i < count; ++i)
    {
        float t = (float)i / (float)(count - 1);
        float angle = center_angle - spread_angle * 0.5f + spread_angle * t;
        out[i] = LogicPolar(angle, speed);
        emitted++;
    }

    return emitted;
}
