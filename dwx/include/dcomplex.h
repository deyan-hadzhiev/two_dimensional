#ifndef __DCOMPLEX_H__
#define __DCOMPLEX_H__

#include <cmath>
#include "constants.h"

template<class T>
class TComplex {
public:
	T r;
	T i;
	explicit TComplex(T real, T imag) noexcept
		: r(real)
		, i(imag)
	{}

	explicit TComplex(T real) noexcept
		: r(real)
		, i(0)
	{}

	TComplex() noexcept
		: r(0)
		, i(0)
	{}

	TComplex(const TComplex<T>&) = default;
	TComplex<T>& operator=(const TComplex<T>&) = default;
	TComplex(TComplex<T>&&) = default;
	TComplex<T>& operator=(TComplex<T>&&) = default;

	inline T& real() noexcept {
		return r;
	}

	template<class T>
	friend T& real(TComplex<T>& z) noexcept;

	inline	T real() const noexcept {
		return r;
	}

	template<class T>
	friend T real(const TComplex<T>& z) noexcept;

	inline T& imag() noexcept {
		return i;
	}

	template<class T>
	friend T& imag(TComplex<T>& z) noexcept;

	inline T imag() const noexcept {
		return i;
	}

	template<class T>
	friend T imag(const TComplex<T>& z) noexcept;

	// return the conjugate of the complex number
	inline TComplex<T> conjugate() const noexcept {
		return TComplex<T>(r, -i);
	}

	template<class T>
	friend TComplex<T> conjugate(const TComplex<T>& z) noexcept;

	// return the magnitude of the complex number
	inline T abs() const noexcept {
		return static_cast<T>(sqrt(r * r + i * i));
	}

	template<class T>
	friend T abs(const TComplex<T>& z) noexcept;

	// return the phase argument
	inline T arg() const noexcept {
		return static_cast<T>(atan2(i, r));
	}

	template<class T>
	friend T arg(const TComplex<T>& z) noexcept;
	
	// return the norm
	inline T norm() const noexcept {
		return r * r + i * i;
	}

	template<class T>
	friend T norm(const TComplex<T>& z) noexcept;

	// operator overloads

	// addition
	inline TComplex<T> operator+(const TComplex<T>& z) const noexcept {
		return TComplex<T>(r + z.r, i + z.i);
	}
	inline TComplex<T> operator+(T rhs) const noexcept {
		return TComplex<T>(r + rhs, i);
	}
	template<class T>
	friend TComplex<T> operator+(T lhs, const TComplex<T>& rhs) noexcept;

	// unary negation
	inline TComplex<T>& operator-() noexcept {
		r = -r;
		i = -i;
		return *this;
	}
	inline TComplex<T> operator-() const noexcept {
		return TComplex<T>(-r, -i);
	}

	// subtraction
	inline TComplex<T> operator-(const TComplex<T>& z) const noexcept {
		return TComplex<T>(r - z.r, i - z.i);
	}
	inline TComplex<T> operator-(T rhs) const noexcept {
		return TComplex<T>(r - rhs, i);
	}

	template<class T>
	friend TComplex<T> operator-(T lhs, const TComplex<T>& rhs) noexcept;

	// multiplication
	inline TComplex<T> operator*(const TComplex<T>& z) const noexcept {
		return TComplex<T>(
			r * z.r - i * z.i,
			r * z.i + i * z.r
			);
	}
	inline TComplex<T> operator*(T rhs) const noexcept {
		return TComplex<T>(r * rhs, i * rhs);
	}

	template<class T>
	friend TComplex<T> operator*(T lhs, const TComplex<T>& rhs) noexcept;

	// divison
	// first the division by scalar since it has more sense as it is used in the division by complex
	inline TComplex<T> operator/(T rhs) const noexcept {
		return TComplex<T>(r / rhs, i / rhs);
	}
	inline TComplex<T> operator/(const TComplex<T>& rhs) const noexcept {
		return TComplex<T>(((*this) * rhs.conjugate()) / rhs.norm());
	}

	template<class T>
	friend TComplex<T> operator/(T lhs, const TComplex<T>& rhs) noexcept;

	const TComplex<T>& operator+=(const TComplex<T>& rhs) noexcept {
		r += rhs.r;
		i += rhs.i;
		return *this;
	}

	const TComplex<T>& operator-=(const TComplex<T>& rhs) noexcept {
		r -= rhs.r;
		i -= rhs.i;
		return *this;
	}

	const TComplex<T>& operator*=(T rhs) noexcept {
		r *= rhs;
		i *= rhs;
		return *this;
	}

	// some friend functions
	template<class T> friend TComplex<T> sqrt(const TComplex<T>& z) noexcept;
	template<class T> friend TComplex<T> log(const TComplex<T>& z) noexcept;
	template<class T> friend TComplex<T> exp(const TComplex<T>& z) noexcept;
	template<class T> friend TComplex<T> pow(const TComplex<T>& z, T p) noexcept;

	// TODO
	// maybe add sin/cos/tan/cot etc?
};

using Complex = TComplex<double>;

template<class T>
inline TComplex<T> operator+(T lhs, const TComplex<T>& rhs) noexcept {
	return rhs + lhs;
}

template<class T>
inline TComplex<T> operator-(T lhs, const TComplex<T>& rhs) noexcept {
	return TComplex<T>(lhs - rhs.r, rhs.i);
}

template<class T>
inline TComplex<T> operator*(T lhs, const TComplex<T>& rhs) noexcept {
	return rhs * lhs;
}

template<class T>
inline TComplex<T> operator/(T lhs, const TComplex<T>& rhs) noexcept {
	return (lhs * rhs.conjugate()) / rhs.norm();
}

template<class T>
inline T& real(TComplex<T>& z) noexcept {
	return z.r;
}

template<class T>
inline T real(const TComplex<T>& z) noexcept {
	return z.r;
}

template<class T>
inline T& imag(TComplex<T>& z) noexcept {
	return z.i;
}

template<class T>
inline T imag(const TComplex<T>& z) noexcept {
	return z.i;
}

template<class T>
inline TComplex<T> conjugate(const TComplex<T>& z) noexcept {
	return z.conjugate();
}

template<class T>
inline T abs(const TComplex<T>& z) noexcept {
	return z.abs();
}

template<class T>
inline T arg(const TComplex<T>& z) noexcept {
	return z.arg();
}

template<class T>
inline T norm(const TComplex<T>& z) noexcept {
	return z.norm();
}


template<class T>
inline TComplex<T> sqrt(const TComplex<T>& z) noexcept {
	const T zAbs = z.abs();
	return (z.i >= 0 ?
		0.5 * TComplex<T>(zAbs + z.r, zAbs - z.r),
		0.5 * TComplex<T>(zAbs + z.r, z.r - zAbs)
		);
}

template<class T>
inline TComplex<T> log(const TComplex<T>& z) noexcept {
	return TComplex<T>(static_cast<T>(log(z.abs())), z.arg());
}

template<class T>
inline TComplex<T> exp(const TComplex<T>& z) noexcept {
	return TComplex<T>(
		static_cast<T>(exp(z.r)) * static_cast<T>(cos(z.i)),
		static_cast<T>(sin(z.i))
		);
}

template<class T>
inline TComplex<T> pow(const TComplex<T>& z, T p) noexcept {
	return exp(p * log(z));
}

#endif
