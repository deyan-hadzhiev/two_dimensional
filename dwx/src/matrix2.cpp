#include "matrix2.h"
#include <math.h>
#include <xmmintrin.h>

Matrix2 operator * (const Matrix2& a, const Matrix2& b) noexcept {
	return Matrix2(
		a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0],
		a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1],
		a.m[1][0] * b.m[0][0] + a.m[1][0] * b.m[1][0],
		a.m[1][0] * b.m[0][1] + a.m[1][0] * b.m[1][1]
		);
}

Matrix2 inverseMatrix(const Matrix2& m) noexcept {
	const float det = determinant(m);
	if (fabs(det) < 1e-12)
		return m;

	const float rdet = 1.0f / det;

	Matrix2 res(m.m[1][1], -m.m[0][1], -m.m[1][0], m.m[0][0]);
	return res * det;
}

Matrix2 transpose(const Matrix2& a) noexcept {
	return Matrix2(
		a.ma[0], a.ma[2],
		a.ma[1], a.ma[3]
	);
}

float determinant(const Matrix2& m) noexcept {
	return m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
}

Matrix2 rotationMatrix(const float angle) noexcept {
	const float C = cosf(angle);
	const float S = sinf(angle);
	return Matrix2(
		C, -S,
		S, C
		);
}
