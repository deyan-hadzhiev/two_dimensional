#ifndef __DRECT_H__
#define __DRECT_H__

#include "vector2.h"
#include "constants.h"

#include <algorithm>
#include <cmath>

// for representing size of a 2d object
class Size2d {
	float w;
	float h;
public:
	Size2d() noexcept
		: w(0.0f)
		, h(0.0f)
	{}

	Size2d(float width, float height) noexcept
		: w(width)
		, h(height)
	{}

	Size2d(const Size2d&) noexcept = default;
	Size2d& operator=(const Size2d&) noexcept = default;

	inline void set(float width, float height) noexcept { w = width; h = height; }
	inline void setWidth(float width) noexcept { w = width; }
	inline void setHeight(float height) noexcept { h = height; }

	inline float getWidth() const noexcept { return w; }
	inline float getHeight() const noexcept {	return h;	}

	Size2d& operator+=(const Size2d& sz) noexcept { w += sz.w; h += sz.h; return *this; }
	Size2d& operator-=(const Size2d& sz) noexcept { w -= sz.w; h -= sz.h; return *this; }
	Size2d& operator/=(float r) noexcept { w /= r; h /= r; return *this; }
	Size2d& operator*=(float r) noexcept { w *= r; h *= r; return *this; }

	friend bool operator==(const Size2d& lhs, const Size2d& rhs);
	friend bool operator!=(const Size2d& lhs, const Size2d& rhs);
	friend Size2d operator+(const Size2d& lhs, const Size2d& rhs);
	friend Size2d operator-(const Size2d& lhs, const Size2d& rhs);
	friend Size2d operator*(const Size2d& lhs, const float);
	friend Size2d operator*(const float, const Size2d& lhs);
	friend Size2d operator/(const Size2d& lhs, const float);
};

inline bool operator==(const Size2d& lhs, const Size2d& rhs) {
	return fabs(lhs.w - rhs.w) <= FEPS && fabs(lhs.h - rhs.h) <= FEPS;
}

inline bool operator!=(const Size2d& lhs, const Size2d& rhs) {
	return !(lhs == rhs);
}

inline Size2d operator+(const Size2d& lhs, const Size2d& rhs) {
	return Size2d(lhs.w + rhs.w, lhs.h + rhs.h);
}

inline Size2d operator-(const Size2d& lhs, const Size2d& rhs) {
	return Size2d(lhs.w - rhs.w, lhs.h - rhs.h);
}

inline Size2d operator*(const Size2d& lhs, const float r) {
	return Size2d(lhs.w * r, lhs.h * r);
}

inline Size2d operator*(const float r, const Size2d& rhs) {
	return Size2d(rhs.w * r, rhs.h * r);
}

inline Size2d operator/(const Size2d& lhs, const float r) {
	return Size2d(lhs.w / r, lhs.h / r);
}

// RECT

// for oparations with rectange in 2d
class Rect {
public:
	float x;
	float y;
	float width;
	float height;

	Rect() noexcept
		: x(0.0f)
		, y(0.0f)
		, width(0.0f)
		, height(0.0f)
	{}

	Rect(float _x, float _y, float _width, float _height) noexcept
		: x(_x)
		, y(_y)
		, width(_width)
		, height(_height)
	{}

	Rect(const Vector2& p, const Size2d& sz) noexcept
		: x(p.x)
		, y(p.y)
		, width(sz.getWidth())
		, height(sz.getHeight())
	{}

	Rect(const Vector2& p0, const Vector2& p1) noexcept
		: x(std::min(p0.x, p1.x))
		, y(std::min(p0.y, p1.y))
		, width(std::fabs(p0.x - p1.x))
		, height(std::fabs(p0.y - p1.y))
	{}

	Rect(const Size2d& sz) noexcept
		: x(0.0f)
		, y(0.0f)
		, width(sz.getWidth())
		, height(sz.getHeight())
	{}

	Rect(const Rect&) noexcept = default;
	Rect& operator=(const Rect&) noexcept = default;

	inline Vector2 getPosition() const noexcept { return Vector2(x, y); }
	inline void setPosition(const Vector2& p) noexcept { x = p.x; y = p.y; }

	inline float getWidth() const noexcept { return width; }
	inline float getHeight() const noexcept { return height; }

	inline void setWidth(float w) noexcept { width = w; }
	inline void setHeight(float h) noexcept { height = h; }

	inline Size2d getSize() const noexcept { return Size2d(width, height); }
	inline void setSize(const Size2d& sz) noexcept { width = sz.getWidth(); height = sz.getHeight(); }

	inline float getLeft()   const noexcept { return x; }
	inline float getTop()    const noexcept { return y; }
	inline float getBottom() const noexcept { return y + height - 1.0f; }
	inline float getRight()  const noexcept { return x + width - 1.0f; }

	inline void setLeft(float left)     noexcept { x = left; }
	inline void setTop(float top)       noexcept { y = top; }
	inline void setBottom(float bottom) noexcept { height = bottom - y + 1.0f; }
	inline void setRight(float right)   noexcept { width = right - y + 1.0f; }

	inline Vector2 getTopLeft() const noexcept { return Vector2(x, y); }
	inline Vector2 getLeftTop() const noexcept { return getTopLeft(); }
	inline void setTopLeft(const Vector2& p) noexcept { x = p.x; y = p.y; }
	inline void setLeftTop(const Vector2& p) noexcept { setTopLeft(p); }

	inline Vector2 getBottomRight() const noexcept { return Vector2(getRight(), getBottom()); }
	inline Vector2 getRightBottom() const noexcept { return getBottomRight(); }
	inline void setBottomRight(const Vector2& p) noexcept { setRight(p.x); setBottom(p.y); }
	inline void setRightBottom(const Vector2& p) noexcept { setBottomRight(p); }

	inline Vector2 getTopRight() const noexcept { return Vector2(getRight(), getTop()); }
	inline Vector2 getRightTop() const noexcept { return getTopRight(); }
	inline void setTopRight(const Vector2& p) noexcept { setRight(p.x); setTop(p.y); }
	inline void setRightTop(const Vector2& p) noexcept { setTopRight(p); }

	inline Vector2 getBottomLeft() const noexcept { return Vector2(getLeft(), getBottom()); }
	inline Vector2 getLeftBottom() const noexcept { return getBottomLeft(); }
	inline void setBottomLeft(const Vector2& p) noexcept { setLeft(p.x); setBottom(p.y); }
	inline void setLeftBottom(const Vector2& p) noexcept { setBottomLeft(p); }

	inline Rect& intersect(const Rect& r) noexcept {
		float x2 = getRight();
		float y2 = getBottom();
		if (x < r.x)
			x = r.x;
		if (y < r.y)
			y = r.y;
		if (x2 > r.getRight())
			width = r.getRight() - x + 1.0f;
		if (y2 > r.getBottom())
			height = r.getBottom() - y + 1.0f;

		if (width <= 0.0f || height <= 0.0f) {
			width = height = 0.0f;
		}
		return *this;
	}

	inline bool intersects(const Rect& r) const noexcept {
		return !(
			(  x +   width  < r.x) ||
			(r.x + r.width  <   x) ||
			(  y +   height < r.y) ||
			(r.y + r.height <   y));
	}

	inline bool inside(const Vector2& p) const noexcept {
		return (
			p.x >= x && p.x <= x + width &&
			p.y >= y && p.y <= y + height
		);
	}
};

#endif // __DRECT_H__
