#ifndef __UTIL_H__
#define __UTIL_H__

#include "constants.h"

inline float signOf(float x) noexcept { return x > 0.f ? 1.f : -1.f; }
inline float sqr(float a) noexcept { return a * a; }
inline float toRadians(float angle) noexcept { return (angle / 180.f) * PI; }
inline float toDegrees(float angle) noexcept { return (angle / PI) * 180.f; }
constexpr inline int nearestInt(float x) noexcept { return static_cast<int>(x > 0 ? x + 0.5f : x - 0.5f); }

template<class T>
T clamp(T x, T xMin, T xMax) noexcept {
	if (xMax < xMin)
		return xMin;
	else if (xMax < x)
		return xMax;
	else if (x < xMin)
		return xMin;
	else
		return x;
}

#endif // __UTIL_H__
