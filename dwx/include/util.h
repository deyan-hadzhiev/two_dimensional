#ifndef __UTIL_H__
#define __UTIL_H__

#include "constants.h"
#include <math.h>

inline float signOf(float x) noexcept { return x > 0.f ? 1.f : -1.f; }
inline float sqr(float a) noexcept { return a * a; }
inline float toRadians(float angle) noexcept { return (angle / 180.f) * PI; }
inline float toDegrees(float angle) noexcept { return (angle / PI) * 180.f; }
constexpr inline int nearestInt(float x) noexcept { return static_cast<int>(x > 0 ? x + 0.5f : x - 0.5f); }

#endif // __UTIL_H__
