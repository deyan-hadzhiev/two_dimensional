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

__declspec(align(1))
class Color {
public:
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		struct { unsigned char r, g, b; };
		unsigned char components[3];
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

	Color(unsigned char _r, unsigned char _g, unsigned char _b) noexcept
		: r(_r)
		, g(_g)
		, b(_b)
	{}

	/// make black
	void makeZero(void) noexcept {
		r = g = b = 0;
	}
	/// set the color explicitly
	void setColor(unsigned char _r, unsigned char _g, unsigned char _b) noexcept {
		r = _r;
		g = _g;
		b = _b;
	}
	/// get the intensity of the color (direct)
	unsigned char intensity(void) const noexcept {
		return unsigned char((int(r) + int(g) + int(b)) / 3) ;
	}
	/// get the perceptual intensity of the color
	unsigned char intensityPerceptual(void) const noexcept {
		return static_cast<unsigned char>(int(r) * 0.299f + int(g) * 0.587f * + int(b) * 0.114f);
	}

	inline const unsigned char& operator[] (int index) const noexcept {
		return components[index];
	}

	inline unsigned char& operator[] (int index) noexcept {
		return components[index];
	}

	void operator += (const Color& rhs) noexcept {
		r = static_cast<unsigned char>(std::min(rhs.r + r, 255));
		g = static_cast<unsigned char>(std::min(rhs.g + g, 255));
		b = static_cast<unsigned char>(std::min(rhs.b + b, 255));
	}
};

#endif // __COLOR_H__
