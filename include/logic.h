#ifndef LOGIC_H
#define LOGIC_H

#include <stdbool.h>

typedef struct LogicVec2
{
    float x;
    float y;
} LogicVec2;

float LogicClamp(float value, float min_value, float max_value);
float LogicApproach(float current, float target, float delta);
float LogicLengthSquared(LogicVec2 value);
float LogicLength(LogicVec2 value);
LogicVec2 LogicAdd(LogicVec2 left, LogicVec2 right);
LogicVec2 LogicSub(LogicVec2 left, LogicVec2 right);
LogicVec2 LogicScale(LogicVec2 value, float scalar);
LogicVec2 LogicNormalize(LogicVec2 value);
LogicVec2 LogicPolar(float angle, float magnitude);
float LogicDistanceSquared(LogicVec2 left, LogicVec2 right);
bool LogicCircleOverlap(LogicVec2 a, float a_radius, LogicVec2 b, float b_radius);
int LogicBuildRing(LogicVec2 *out, int capacity, int count, float start_angle, float speed);
int LogicBuildFanToward(LogicVec2 *out, int capacity, int count, LogicVec2 origin, LogicVec2 target,
                        float spread_angle, float speed);

#endif
