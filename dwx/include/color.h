#ifndef __COLOR_H__
#define __COLOR_H__

#include <math.h>
#include <algorithm>
#include "util.h"

inline unsigned convertTo8bit(float x) noexcept {
	if (x < 0)
		x = 0;
	else if (x > 1)
		x = 1;
	return nearestInt(x * 255.0f);
}

inline unsigned convertTo8bit_sRGB(float x) noexcept {
	const float a = 0.055f;
	if (x <= 0)
		return 0;
	else if (x >= 1)
		return 255;
	// sRGB transform:
	if (x <= 0.0031308f)
		x = x * 12.02f;
	else
		x = (1.0f + a) * powf(x, 1.0f / 2.4f) - a;
	return nearestInt(x * 255.0f);
}

void initColor() noexcept;
unsigned convertTo8bit_sRGB_cached(float x) noexcept;

/// Represents a color, using floatingpoint components in [0..1]
class FloatColor {
public:
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		struct { float r, g, b; };
		float components[3];
	};
	//
	FloatColor() noexcept
		: FloatColor(0.0f)
	{}
	//!< Construct a color from floatingpoint values
	FloatColor(float _r, float _g, float _b) noexcept {
		setColor(_r, _g, _b);
	}

	explicit FloatColor(float v)	noexcept {
		setColor(v, v, v);
	}
	//!< Construct a color from R8G8B8 value like "0xffce08"
	explicit FloatColor(unsigned rgbcolor) noexcept {
		const float divider = 1.0f / 255.0f;
		b = (rgbcolor & 0xff) * divider;
		g = ((rgbcolor >> 8) & 0xff) * divider;
		r = ((rgbcolor >> 16) & 0xff) * divider;
	}

	FloatColor(unsigned char _r, unsigned char _g, unsigned char _b) noexcept {
		const float divider = 1.0f / 255.0f;
		r = static_cast<float>(_r * divider);
		g = static_cast<float>(_g * divider);
		b = static_cast<float>(_b * divider);
	}
	/// convert to RGB32, with channel shift specifications. The default values are for
	/// the blue channel occupying the least-significant byte
	unsigned toRGB32(int redShift = 16, int greenShift = 8, int blueShift = 0) const noexcept {
		unsigned ir = convertTo8bit_sRGB_cached(r);
		unsigned ig = convertTo8bit_sRGB_cached(g);
		unsigned ib = convertTo8bit_sRGB_cached(b);
		return (ib << blueShift) | (ig << greenShift) | (ir << redShift);
	}
	/// make black
	void makeZero(void) noexcept {
		r = g = b = 0;
	}
	/// set the color explicitly
	void setColor(float _r, float _g, float _b) noexcept {
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	float intensity(void) const noexcept {
		return (r + g + b) * 0.3333333333f;
	}
	/// get the perceptual intensity of the color
	float intensityPerceptual(void) const noexcept {
		return (r * 0.299f + g * 0.587f + b * 0.114f);
	}
	/// Accumulates some color to the current
	void operator += (const FloatColor& rhs) noexcept {
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
	}
	/// multiplies the color
	void operator *= (float multiplier) noexcept {
		r *= multiplier;
		g *= multiplier;
		b *= multiplier;
	}
	/// divides the color
	void operator /= (float divider) noexcept {
		float rdivider = 1.0f / divider;
		r *= rdivider;
		g *= rdivider;
		b *= rdivider;
	}
	// 0 = desaturate; 1 = don't change
	void adjustSaturation(float amount) noexcept {
		float mid = intensity();
		r = r * amount + mid * (1 - amount);
		g = g * amount + mid * (1 - amount);
		b = b * amount + mid * (1 - amount);
	}

	inline const float& operator[] (int index) const noexcept {
		return components[index];
	}

	inline float& operator[] (int index) noexcept {
		return components[index];
	}
};

/// adds two colors
inline FloatColor operator + (const FloatColor& a, const FloatColor& b) noexcept {
	return FloatColor(a.r + b.r, a.g + b.g, a.b + b.b);
}

/// subtracts two colors
inline FloatColor operator - (const FloatColor& a, const FloatColor& b) noexcept {
	return FloatColor(a.r - b.r, a.g - b.g, a.b - b.b);
}

/// multiplies two colors
inline FloatColor operator * (const FloatColor& a, const FloatColor& b) noexcept {
	return FloatColor(a.r * b.r, a.g * b.g, a.b * b.b);
}

/// multiplies a color by some multiplier
inline FloatColor operator * (const FloatColor& a, float multiplier) noexcept {
	return FloatColor(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// multiplies a color by some multiplier
inline FloatColor operator * (float multiplier, const FloatColor& a) noexcept {
	return FloatColor(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// divides some color
inline FloatColor operator / (const FloatColor& a, float divider) noexcept {
	float mult = 1.0f / divider;
	return FloatColor(a.r * mult, a.g * mult, a.b * mult);
}

#pragma pack(push, 1)
template<class T>
class TColor {
public:
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		struct { T r, g, b; };
		T components[3];
	};
	//
	TColor() noexcept
		: r(0)
		, g(0)
		, b(0)
	{}

	explicit TColor(T _r, T _g, T _b) noexcept
		: r(_r)
		, g(_g)
		, b(_b)
	{}

	/// make black
	void makeZero() noexcept {
		r = g = b = 0;
	}
	/// set the color explicitly
	void setColor(T _r, T _g, T _b) noexcept {
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	T intensity() const noexcept {
		return static_cast<T>((int64(r) + int64(g) + int64(b)) / 3);
	}
	/// get the perceptual intensity of the color
	T intensityPerceptual() const noexcept {
		return static_cast<T>(int64(r) * 0.299f + int64(g) * 0.587f + int64(b) * 0.114f);
	}

	inline const T& operator[] (int index) const noexcept {
		return components[index];
	}

	inline T& operator[] (int index) noexcept {
		return components[index];
	}

	TColor& operator += (const TColor& rhs) noexcept {
		r = r + rhs.r;
		g = g + rhs.g;
		b = b + rhs.b;
		return *this;
	}

	TColor& operator -= (const TColor& rhs) noexcept {
		r = r - rhs.r;
		g = g - rhs.g;
		b = b - rhs.b;
		return *this;
	}

	TColor& operator *= (const double mult) noexcept {
		r = static_cast<T>(static_cast<double>(r) * mult);
		g = static_cast<T>(static_cast<double>(g) * mult);
		b = static_cast<T>(static_cast<double>(b) * mult);
		return *this;
	}

	TColor& operator /= (const double div) noexcept {
		const float mult = 1.0f / div;
		r = static_cast<T>(static_cast<double>(r) * mult);
		g = static_cast<T>(static_cast<double>(g) * mult);
		b = static_cast<T>(static_cast<double>(b) * mult);
		return *this;
	}
};
#pragma pack(pop)

template<class T>
inline TColor<T> operator+(const TColor<T>& lhs, const TColor<T>& rhs) noexcept {
	return TColor<T>(
		lhs.r + rhs.r,
		lhs.g + rhs.g,
		lhs.b + rhs.b
		);
}

template<class T>
inline TColor<T> operator-(const TColor<T>& lhs, const TColor<T>& rhs) noexcept {
	return TColor<T>(
		lhs.r - rhs.r,
		lhs.g - rhs.g,
		lhs.b - rhs.b
		);
}

template<class T>
inline TColor<T> operator*(const TColor<T>& lhs, const double mult) noexcept {
	return TColor<T>(
		static_cast<T>(lhs.r * mult),
		static_cast<T>(lhs.g * mult),
		static_cast<T>(lhs.b * mult)
		);
}

template<class T>
inline TColor<T> operator*(const double mult, const TColor<T>& rhs) noexcept {
	return TColor<T>(
		static_cast<T>(rhs.r * mult),
		static_cast<T>(rhs.g * mult),
		static_cast<T>(rhs.b * mult)
		);
}

template<class T>
inline TColor<T> operator/(const TColor<T>& lhs, const double div) noexcept {
	const double mult = 1.0f / div;
	return TColor<T>(
		static_cast<T>(lhs.r * mult),
		static_cast<T>(lhs.g * mult),
		static_cast<T>(lhs.b * mult)
		);
}

#pragma pack(push, 1)
class Color {
public:
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		struct { uint8 r, g, b; };
		uint8 components[3];
	};
	//
	Color() noexcept
		: r(0)
		, g(0)
		, b(0)
	{}

	//!< Construct a color from R8G8B8 value like "0xffce08"
	explicit Color(unsigned rgbcolor) noexcept {
		b = (rgbcolor & 0xff);
		g = ((rgbcolor >> 8) & 0xff);
		r = ((rgbcolor >> 16) & 0xff);
	}

	template<class T>
	explicit Color(const TColor<T>& _tc)
		: r(static_cast<uint8>(clamp(_tc.r, static_cast<T>(0), static_cast<T>(255))))
		, g(static_cast<uint8>(clamp(_tc.g, static_cast<T>(0), static_cast<T>(255))))
		, b(static_cast<uint8>(clamp(_tc.b, static_cast<T>(0), static_cast<T>(255))))
	{}

	template<>
	Color(const TColor<double>& _tc)
		: r(static_cast<uint8>(clamp(_tc.r * 255.0, 0.0, 255.0)))
		, g(static_cast<uint8>(clamp(_tc.g * 255.0, 0.0, 255.0)))
		, b(static_cast<uint8>(clamp(_tc.b * 255.0, 0.0, 255.0)))
	{}

	template<>
	explicit Color(const TColor<Complex>& _tc)
		: r(static_cast<uint8>(clamp(std::abs(_tc.r) * 255.0, 0.0, 255.0)))
		, g(static_cast<uint8>(clamp(std::abs(_tc.g) * 255.0, 0.0, 255.0)))
		, b(static_cast<uint8>(clamp(std::abs(_tc.b) * 255.0, 0.0, 255.0)))
	{}

	explicit Color(uint8 _r, uint8 _g, uint8 _b) noexcept
		: r(_r)
		, g(_g)
		, b(_b)
	{}

	/// make black
	void makeZero(void) noexcept {
		r = g = b = 0;
	}
	/// set the color explicitly
	void setColor(uint8 _r, uint8 _g, uint8 _b) noexcept {
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	uint8 intensity(void) const noexcept {
		return static_cast<uint8>((uint16(r) + uint16(g) + uint16(b)) / 3) ;
	}
	/// get the perceptual intensity of the color
	uint8 intensityPerceptual(void) const noexcept {
		return static_cast<unsigned char>(int(r) * 0.299f + int(g) * 0.587f + int(b) * 0.114f);
	}

	inline const uint8& operator[] (int index) const noexcept {
		return components[index];
	}

	inline uint8& operator[] (int index) noexcept {
		return components[index];
	}

	Color& operator += (const Color& rhs) noexcept {
		r = static_cast<uint8>(std::min(static_cast<uint16>(r) + static_cast<uint16>(rhs.r), 0xff));
		g = static_cast<uint8>(std::min(static_cast<uint16>(g) + static_cast<uint16>(rhs.g), 0xff));
		b = static_cast<uint8>(std::min(static_cast<uint16>(b) + static_cast<uint16>(rhs.b), 0xff));
		return *this;
	}

	Color& operator -= (const Color& rhs) noexcept {
		r = static_cast<uint8>(std::max(static_cast<int16>(r) - static_cast<int16>(rhs.r), 0));
		g = static_cast<uint8>(std::max(static_cast<int16>(g) - static_cast<int16>(rhs.g), 0));
		b = static_cast<uint8>(std::max(static_cast<int16>(b) - static_cast<int16>(rhs.b), 0));
		return *this;
	}

	Color& operator *= (const double mult) noexcept {
		r = static_cast<uint8>(clamp(r * mult, 0.0, 255.0));
		g = static_cast<uint8>(clamp(g * mult, 0.0, 255.0));
		b = static_cast<uint8>(clamp(b * mult, 0.0, 255.0));
		return *this;
	}

	Color& operator /= (const double div) noexcept {
		const double mult = 1.0f / div;
		r = static_cast<uint8>(clamp(r * mult, 0.0, 255.0));
		g = static_cast<uint8>(clamp(g * mult, 0.0, 255.0));
		b = static_cast<uint8>(clamp(b * mult, 0.0, 255.0));
		return *this;
	}

	template<class T>
	explicit operator TColor<T>() const {
		return TColor<T>(
			static_cast<T>(r),
			static_cast<T>(g),
			static_cast<T>(b)
			);
	}

	explicit operator TColor<double>() const {
		return TColor<double>(
			static_cast<double>(r) / 255.0,
			static_cast<double>(g) / 255.0,
			static_cast<double>(b) / 255.0
			);
	}

	explicit operator TColor<Complex>() const {
		return TColor<Complex>(
			Complex(static_cast<double>(r / 255.0), 0.0),
			Complex(static_cast<double>(g / 255.0), 0.0),
			Complex(static_cast<double>(b / 255.0), 0.0)
			);
	}
};
#pragma pack(pop)

inline Color operator+(const Color& lhs, const Color& rhs) noexcept {
	return Color(
		static_cast<uint8>(std::min(static_cast<uint16>(lhs.r) + static_cast<uint16>(rhs.r), 0xff)),
		static_cast<uint8>(std::min(static_cast<uint16>(lhs.g) + static_cast<uint16>(rhs.g), 0xff)),
		static_cast<uint8>(std::min(static_cast<uint16>(lhs.b) + static_cast<uint16>(rhs.b), 0xff))
		);
}

inline Color operator-(const Color& lhs, const Color& rhs) noexcept {
	return Color(
		static_cast<uint8>(std::max(static_cast<int16>(lhs.r) - static_cast<int16>(rhs.r), 0)),
		static_cast<uint8>(std::max(static_cast<int16>(lhs.g) - static_cast<int16>(rhs.g), 0)),
		static_cast<uint8>(std::max(static_cast<int16>(lhs.b) - static_cast<int16>(rhs.b), 0))
		);
}

inline Color operator*(const Color& lhs, const double mult) noexcept {
	return Color(
		static_cast<uint8>(clamp(lhs.r * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(lhs.g * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(lhs.b * mult, 0.0, 255.0))
		);
}

inline Color operator*(const double mult, const Color& rhs) noexcept {
	return Color(
		static_cast<uint8>(clamp(rhs.r * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(rhs.g * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(rhs.b * mult, 0.0, 255.0))
		);
}

inline Color operator/(const Color& lhs, const double div) noexcept {
	const double mult = 1.0f / div;
	return Color(
		static_cast<uint8>(clamp(lhs.r * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(lhs.g * mult, 0.0, 255.0)),
		static_cast<uint8>(clamp(lhs.b * mult, 0.0, 255.0))
		);
}

#endif // __COLOR_H__
