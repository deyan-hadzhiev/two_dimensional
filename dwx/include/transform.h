#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "vector4.h"
#include "matrix4.h"
#include "util.h"

/// A transformation class, which implements model-view transform. Objects can be
/// arbitrarily scaled, rotated and translated.
class Transform {
public:
	Transform() noexcept { reset(); }

	void reset() noexcept {
		transform = Matrix4(1);
		// old code inversed an identity matrix - the inverse is the identity matrix itself
		inverseTransform = transform; //inverseMatrix(transform);
		offset.makeZero();
	}

	void scale(float X, float Y, float Z, float W) noexcept {
		Matrix4 scaling(X);
		scaling.m[1][1] = Y;
		scaling.m[2][2] = Z;
		scaling.m[3][3] = W;

		transform = transform * scaling;
		inverseTransform = inverseMatrix(transform);
	}

	void rotate(float xy, float yz, float zx, float xw, float yw, float zw) noexcept {
		transform = transform *
			rotationAroundXY(toRadians(xy)) *
			rotationAroundYZ(toRadians(yz)) *
			rotationAroundZX(toRadians(zx)) *
			rotationAroundXW(toRadians(xw)) *
			rotationAroundYW(toRadians(yw)) *
			rotationAroundZW(toRadians(zw));
		inverseTransform = inverseMatrix(transform);
	}

	void translate(const Vector4& V) noexcept {
		offset = V;
	}

	Vector4 point(Vector4 P) const noexcept {
		P = P * transform;
		P = P + offset;

		return P;
	}

	Vector4 undoPoint(Vector4 P) const noexcept {
		P = P - offset;
		P = P * inverseTransform;

		return P;
	}

	Vector4 direction(const Vector4& dir) const noexcept {
		return dir * transform;
	}

	Vector4 normal(const Vector4& dir) const noexcept {
		const Matrix4 transposedInverse = transpose(inverseTransform);
		return dir * transposedInverse;
	}

	Vector4 undoDirection(const Vector4& dir) const noexcept {
		return dir * inverseTransform;
	}

	Ray ray(const Ray& inputRay) const noexcept {
		Ray result = inputRay;
		result.start = point(inputRay.start);
		result.dir = direction(inputRay.dir);
		return result;
	}

	Ray undoRay(const Ray& inputRay) const noexcept {
		Ray result = inputRay;
		result.start = undoPoint(inputRay.start);
		result.dir = undoDirection(inputRay.dir);
		return result;
	}

	Matrix4 getMatrix() const noexcept {
		return transform;
	}

	void setMatrix(const Matrix4& m) noexcept {
		transform = m;
		inverseTransform = inverseMatrix(transform);
	}

private:
	Matrix4 transform;
	Matrix4 inverseTransform;
	Vector4 offset;
};

#endif // __TRANSFORM_H__
