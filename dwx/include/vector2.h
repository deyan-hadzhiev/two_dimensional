#ifndef __VECTOR2_H__
#define __VECTOR2_H__

#include <math.h>

class Vector2 {
public:
	union {
		struct { float x, y; };
		float components[2];
	};
	//////////////////////////////////////
	Vector2() noexcept
		: Vector2(0.f, 0.f)
	{}

	Vector2(float _x, float _y) noexcept
		: x(_x)
		, y(_y)
	{}

	void set(float _x, float _y) noexcept {
		x = _x;
		y = _y;
	}

	void makeZero() noexcept {
		x = y = 0.0f;
	}

	inline float length() const noexcept {
		return sqrtf(x * x + y * y);
	}

	inline float lengthSqr() const noexcept {
		return x * x + y * y;
	}

	void scale(float multiplier)  noexcept {
		x *= multiplier;
		y *= multiplier;
	}

	void operator *= (float multiplier) noexcept {
		scale(multiplier);
	}

	void operator += (const Vector2& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
	}

	void operator /= (float divider) noexcept {
		scale(1.f / divider);
	}

	void normalize() noexcept {
		const float multiplier = 1.f / length();
		scale(multiplier);
	}

	void setLength(float newLength) noexcept {
		scale(newLength / length());
	}

	inline float& operator[] (int index) noexcept {
		return components[index];
	}

	inline const float& operator[] (int index) const noexcept {
		return components[index];
	}

	int maxDimension() const noexcept {
		int bi = 0;
		double maxD = fabs(x);
		if (fabs(y) > maxD) { maxD = fabs(y); bi = 1; }
		return bi;
	}
};

inline Vector2 operator + (const Vector2& lhs, const Vector2& rhs) noexcept {
	return Vector2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline Vector2 operator - (const Vector2& lhs, const Vector2& rhs) noexcept {
	return Vector2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline Vector2 operator - (const Vector2 a) noexcept {
	return Vector2(-a.x, -a.y);
}

// dot product
inline float operator * (const Vector2& lhs, const Vector2& rhs) noexcept {
	return lhs.x * rhs.x + lhs.y * rhs.y;
}

// explicit dot product
inline float dot(const Vector2& lhs, const Vector2& rhs) noexcept {
	return lhs.x * rhs.x + lhs.y * rhs.y;
}

// gives the oriented area of the parallelogram described by the two vectors
// NOTE: divide by 2 to get the triangle area defined by the two vecors
inline float operator ^ (const Vector2& lhs, const Vector2& rhs) noexcept {
	return lhs.x * rhs.y - lhs.y * rhs.x;
}

inline Vector2 operator * (const Vector2& lhs, float multiplier) noexcept {
	return Vector2(lhs.x * multiplier, lhs.y * multiplier);
}

inline Vector2 operator * (float multiplier, const Vector2& rhs) noexcept {
	return Vector2(rhs.x * multiplier, rhs.y * multiplier);
}

inline Vector2 operator / (const Vector2& lhs, float divider) noexcept {
	const float multiplier = 1.0f / divider;
	return Vector2(lhs.x * multiplier, lhs.y * multiplier);
}

inline Vector2 reflect(const Vector2& ray, const Vector2& norm) noexcept {
	Vector2 result = ray - 2 * dot(ray, norm) * norm;
	result.normalize();
	return result;
}

inline Vector2 project(const Vector2& v, int a, int b) {
	Vector2 result;
	result[a] = v[0];
	result[b] = v[1];
	return result;
}

inline Vector2 unproject(const Vector2& v, int a, int b) {
	Vector2 result;
	result[0] = v[a];
	result[1] = v[b];
	return result;
}

#endif // __VECTOR2_H__
