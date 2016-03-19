#ifndef __VECTOR4_H__
#define __VECTOR4_H__

#include <math.h>

__declspec( align(16) )
class Vector4 {
public:
	union {
		struct { float x, y, z, w; };
		float components[4];
	};
	//////////////////////////////////////
	Vector4() noexcept
		: Vector4(0.f, 0.f, 0.f, 0.f)
	{}

	Vector4(float _x, float _y, float _z, float _w) noexcept
		: x(_x)
		, y(_y)
		, z(_z)
		, w(_w)
	{}

	void set(float _x, float _y, float _z, float _w) noexcept {
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	void makeZero() noexcept {
		x = y = z = z = 0.f;
	}

	inline float length() const noexcept {
		return sqrtf(x * x + y * y + z * z + w * w);
	}

	inline float lengthSqr() const noexcept {
		return x * x + y * y + z * z + w * w;
	}

	void scale(float multiplier)  noexcept {
		x *= multiplier;
		y *= multiplier;
		z *= multiplier;
		w *= multiplier;
	}

	void operator *= (float multiplier) noexcept {
		scale(multiplier);
	}

	void operator += (const Vector4& rhs) noexcept {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
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
		if (fabs(z) > maxD) { maxD = fabs(z); bi = 2; }
		if (fabs(w) > maxD) { maxD = fabs(w); bi = 3; }
		return bi;
	}
};

inline Vector4 operator + (const Vector4& lhs, const Vector4& rhs) noexcept {
	return Vector4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}

inline Vector4 operator - (const Vector4& lhs, const Vector4& rhs) noexcept {
	return Vector4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}

inline Vector4 operator - (const Vector4 a) noexcept {
	return Vector4(-a.x, -a.y, -a.z, -a.w);
}

// dot product
inline float operator * (const Vector4& lhs, const Vector4& rhs) noexcept {
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

// explicit dot product
inline float dot(const Vector4& lhs, const Vector4& rhs) noexcept {
	return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

// cross3 product (would be nice to have operator, but there are no ternary operator overloadings in C++ :(
// kudos to :
// http://steve.hollasch.net/thesis/chapter2.html#s2.1
// naively enough the 4D cross product may be represented as the given determinant
//                   | ex ey ez ew |
// cross4(U, V, W) = | Ux Uy Uz Uw |
//                   | Vx Vy Vz Vw |
//                   | Wx Wy Wz Ww |
// which using minor and some optimizations leads to the following formula:
// but generally - cross products should be avoided if possible
inline Vector4 cross4(const Vector4& u, const Vector4& v, const Vector4& w) noexcept {
	// intermediate values
	const float a = (v.x * w.y) - (v.y * w.x);
	const float b = (v.x * w.z) - (v.z * w.x);
	const float c = (v.x * w.w) - (v.w * w.x);
	const float d = (v.y * w.z) - (v.z * w.y);
	const float e = (v.y * w.w) - (v.w * w.y);
	const float f = (v.z * w.w) - (v.w * w.z);
	// calculate the result vector
	return Vector4(
		  (u.y * f) - (u.z * e) + (u.w * d),
		- (u.x * f) + (u.z * c) - (u.w * b),
		  (u.x * e) - (u.y * c) + (u.w * a),
		- (u.x * d) + (u.y * b) - (u.z * a)
		);
}

inline Vector4 operator * (const Vector4& lhs, float multiplier) noexcept {
	return Vector4(lhs.x * multiplier, lhs.y * multiplier, lhs.z * multiplier, lhs.w * multiplier);
}

inline Vector4 operator * (float multiplier, const Vector4& rhs) noexcept {
	return Vector4(rhs.x * multiplier, rhs.y * multiplier, rhs.z * multiplier, rhs.w * multiplier);
}

inline Vector4 operator / (const Vector4& lhs, float divider) noexcept {
	const float multiplier = 1.0f / divider;
	return Vector4(lhs.x * multiplier, lhs.y * multiplier, lhs.z * multiplier, lhs.w * multiplier);
}

inline Vector4 reflect(const Vector4& ray, const Vector4& norm) noexcept {
	Vector4 result = ray - 2 * dot(ray, norm) * norm;
	result.normalize();
	return result;
}

inline Vector4 faceforward(const Vector4& ray, const Vector4& norm) noexcept {
	if (dot(ray, norm) < 0)
		return norm;
	else
		return -norm;
}

inline Vector4 project(const Vector4& v, int a, int b, int c, int d) {
	Vector4 result;
	result[a] = v[0];
	result[b] = v[1];
	result[c] = v[2];
	result[d] = v[3];
	return result;
}

inline Vector4 unproject(const Vector4& v, int a, int b, int c, int d) {
	Vector4 result;
	result[0] = v[a];
	result[1] = v[b];
	result[2] = v[c];
	result[3] = v[d];
	return result;
}

// flags that mark a ray in some way, so the behaviour of the raytracer can be altered.
enum RayFlags {
	// RF_DEBUG - the ray is a debug one (launched from a mouse-click on the rendered image).
	// raytrace() prints diagnostics when it encounters such a ray.
	RF_DEBUG = 0x0001,

	// RF_SHADOW - the ray is a shadow ray. This hints the raytracer to skip some calculations
	// (since the IntersectionData won't be used for shading), and to disable backface culling
	// for Mesh objects.
	RF_SHADOW = 0x0002,

	// RF_GLOSSY - the ray has hit some glossy surface somewhere along the way.
	// so if it meets a new glossy surface, it can safely use lower sampling settings.
	RF_GLOSSY = 0x0004,

	// last constituent of a ray path was a diffuse surface
	RF_DIFFUSE = 0x0008,
};

__declspec( align(16) )
class Ray {
public:
	Vector4 start, dir;
	unsigned flags;
	int depth;
	Ray() noexcept
		: flags(0)
		, depth(0)
	{}

	Ray(const Vector4& _start, const Vector4& _dir) noexcept
		: start(_start)
		, dir(_dir)
		, flags(0)
		, depth(0)
	{}
};

inline Ray project(Ray r, int a, int b, int c, int d) noexcept {
	r.start = project(r.start, a, b, c, d);
	r.dir = project(r.dir, a, b, c, d);
	return r;
}

inline void orthonormedSystem(const Vector4& a, Vector4& b, Vector4& c, Vector4& d) noexcept {
	const Vector4 aa(fabs(a.x), fabs(a.y), fabs(a.z), fabs(a.w));
	int less1 = aa.x<aa.y?0:1;
	int less2 = aa.z<aa.w?2:3;
	int fst;
	if(aa[less1]<aa[less2]) {
		fst=less1;
		less1=1-less1;
	}
	else {
		fst=less2;
		less2=3-(less2&0x1);
	}
	int snd = aa[less1]<aa[less2]? less1:less2;
	b.makeZero(); b[less1] = 1.f;
	c.makeZero(); c[less2] = 1.f;
	d = cross4(a,b,c); d.normalize();
	b = cross4(c,d,a); b.normalize();
	c = cross4(d,a,b); c.normalize();
}

// this is extended version of refraction for three-dimensions
inline Vector4 refract(const Vector4& i, const Vector4& n, float ior) noexcept {
	const float NdotI = dot(i, n);
	const float k = 1 - (ior * ior) * (1 - NdotI * NdotI);
	if (k < 0)
		return Vector4(0, 0, 0, 0);
	return ior * i - (ior * NdotI + sqrtf(k)) * n;
}

#endif // __VECTOR4_H__
