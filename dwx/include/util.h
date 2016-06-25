#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <complex>
#include "constants.h"

#ifdef DDEBUG
#define DASSERT(expr) \
		do { \
			if (!(expr)) \
				__debugbreak(); \
		} while (0)
#else
#define DASSERT(expr)
#endif // DDEBUG

static const float FEPS = 1e-8f;
static const double PI = 3.141592653589793238462643383279502884197169399375105820974944;

using Complex = std::complex<double>;

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
inline float toRadians(float angle) noexcept { return static_cast<float>((angle / 180.0) * PI); }
inline float toDegrees(float angle) noexcept { return static_cast<float>((angle / PI) * 180.0); }
inline double toRadians(double angle) noexcept { return (angle / 180.0) * PI; }
inline double toDegrees(double angle) noexcept { return (angle / PI) * 180.0; }
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

#endif // __UTIL_H__
