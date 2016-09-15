#ifndef __VECTORN_H__
#define __VECTORN_H__

#include "util.h"

template<int d, class T>
class Vector {
public:
	T c[d];

	Vector() noexcept
		: c {}
	{}

	Vector(const T _c[])
		: c {}
	{
		for (int i = 0; i < d; ++i) {
			c[i] = _c[i];
		}
	}

	inline void set(int i, T value) {
		DASSERT(i >= 0 && i < d);
		c[i] = value;
	}

	inline void makeZero() noexcept {
		for (int i = 0; i < d; ++i) {
			c[i] = T();
		}
	}

	inline double length() const noexcept {
		return sqrt(static_cast<double>(lengthSqr));
	}

	inline T lengthSqr() const noexcept {
		T sum = 0;
		for (int i = 0; i < d; ++i) {
			sum += c[i] * c[i];
		}
		return sum;
	}

	inline void scale(double m) noexcept {
		for (int i = 0; i < d; ++i) {
			c[i] = static_cast<T>(c[i] * m);
		}
	}

	inline void operator *= (double m) noexcept {
		scale(m);
	}

	inline void operator += (const Vector<d, T>& rhs) noexcept {
		for (int i = 0; i < d; ++i) {
			c[i] += rhs.c[i];
		}
	}

	inline void operator -= (const Vector<d, T>& rhs) noexcept {
		for (int i = 0; i < d; ++i) {
			c[i] -= rhs.c[i];
		}
	}

	inline void operator /= (double divider) noexcept {
		scale(1.0 / divider);
	}

	inline void normalize() noexcept {
		const double multiplier = 1.0 / length();
		scale(multiplier);
	}

	inline void setLength(double newLength) noexcept {
		scale(newLength / length());
	}

	inline T& operator [] (int index) noexcept {
		DASSERT(index >= 0 && index < d);
		return c[index];
	}

	inline const T& operator [] (int index) const noexcept {
		DASSERT(index >= 0 && index < d);
		return c[index];
	}

	int maxDimension() const noexcept {
		int mi = 0;
		T maxD = abs(c[0]);
		for (int i = 1; i < d; ++i) {
			const T ai = abs(c[i]);
			if (ai > maxD) {
				maxD = ai;
				mi = i;
			}
		}
		return mi;
	}
};

template<int d, class T>
inline Vector<d, T> operator + (const Vector<d, T>& lhs, const Vector<d, T>& rhs) noexcept {
	Vector<d, T> r(lhs);
	r += rhs;
	return r;
}

template<int d, class T>
inline Vector<d, T> operator - (const Vector<d, T>& lhs, const Vector<d, T>& rhs) noexcept {
	Vector<d, T> r(lhs);
	r -= rhs;
	return r;
}

template<int d, class T>
inline Vector<d, T> operator - (Vector<d, T> a) noexcept {
	for (int i = 0; i < d; ++i) {
		a.c[i] = -a.c[i];
	}
	return a;
}

template<int d, class T>
inline Vector<d, T> operator * (const Vector<d, T>& lhs, double multiplier) noexcept {
	Vector<d, T> r(lhs);
	r.scale(multiplier);
	return r;
}

template<int d, class T>
inline Vector<d, T> operator * (double multiplier, const Vector<d, T>& lhs) noexcept {
	Vector<d, T> r(lhs);
	r.scale(multiplier);
	return r;
}

template<int d, class T>
inline Vector<d, T> operator / (const Vector<d, T>& lhs, double divider) noexcept {
	Vector<d, T> r(lhs);
	r.scale(1.0 / divider);
	return r;
}

template<int d, class T>
inline T dot(const Vector<d, T>& lhs, const Vector<d, T>& rhs) {
	T sum = T();
	for (int i = 0; i < d; ++i) {
		sum += lhs.c[i] * rhs.c[i];
	}
	return sum;
}

#endif // __VECTORN_H__
