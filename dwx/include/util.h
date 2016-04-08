#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <string>
#include <vector>
#include "constants.h"

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8  = uint8_t;
using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8  = int8_t;

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

std::vector<std::string> splitString(const char *str, char c = ' ');

template<class T>
std::vector<T> convolute(const std::vector<T>& input, std::vector<float> vec);

struct Extremum {
	int start;
	int end;
	int d; //!< will be positive if the extremum is maximum and negative if it is minimum
};

template<class T>
std::vector<Extremum> findExtremums(const std::vector<T>& input);

#endif // __UTIL_H__
