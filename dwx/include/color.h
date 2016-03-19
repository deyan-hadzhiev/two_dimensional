#ifndef __COLOR_H__
#define __COLOR_H__

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
class Color {
public:
	// a union, that allows us to refer to the channels by name (::r, ::g, ::b),
	// or by index (::components[0] ...). See operator [].
	union {
		struct { float r, g, b; };
		float components[3];
	};
	//
	Color() noexcept {}
	//!< Construct a color from floatingpoint values
	Color(float _r, float _g, float _b) noexcept {
		setColor(_r, _g, _b);
	}
	//!< Construct a color from R8G8B8 value like "0xffce08"
	explicit Color(unsigned rgbcolor) noexcept {
		const float divider = 1.0f / 255.0f;
		b = (rgbcolor & 0xff) * divider;
		g = ((rgbcolor >> 8) & 0xff) * divider;
		r = ((rgbcolor >> 16) & 0xff) * divider;
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
	void operator += (const Color& rhs) noexcept {
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
inline Color operator + (const Color& a, const Color& b) noexcept {
	return Color(a.r + b.r, a.g + b.g, a.b + b.b);
}

/// subtracts two colors
inline Color operator - (const Color& a, const Color& b) noexcept {
	return Color(a.r - b.r, a.g - b.g, a.b - b.b);
}

/// multiplies two colors
inline Color operator * (const Color& a, const Color& b) noexcept {
	return Color(a.r * b.r, a.g * b.g, a.b * b.b);
}

/// multiplies a color by some multiplier
inline Color operator * (const Color& a, float multiplier) noexcept {
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// multiplies a color by some multiplier
inline Color operator * (float multiplier, const Color& a) noexcept {
	return Color(a.r * multiplier, a.g * multiplier, a.b * multiplier);
}

/// divides some color
inline Color operator / (const Color& a, float divider) noexcept {
	float mult = 1.0f / divider;
	return Color(a.r * mult, a.g * mult, a.b * mult);
}

#endif // __COLOR_H__
