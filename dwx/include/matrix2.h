#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector2.h"

__declspec( align(16) )
class Matrix2 {
public:
	union	{
		float ma[4];
		float m[2][2];
		Vector2 rows[2];
	};

	Matrix2() noexcept {}
	Matrix2(float diagonalElement) noexcept
		: ma{ 0 }
	{
		m[0][0] = m[1][1] = diagonalElement;
	}

	Matrix2(const float arr[4]) noexcept
		: ma{
		arr[0x0], arr[0x1],
		arr[0x2], arr[0x3]
	}	{}

	Matrix2(
		float ax0, float ax1,
		float ax2, float ax3
		) noexcept
		: ma{
		ax0, ax1,
		ax2, ax3,
	}
	{}
};

inline Matrix2 operator * (const float s, const Matrix2& m) noexcept {
	return Matrix2(
		m.ma[0] * s,
		m.ma[1] * s,
		m.ma[2] * s,
		m.ma[3] * s
	);
}

// TODO: Some day optimize with some intrinsic calls - but lets have some basic stuff for the begining
inline Vector2 operator * (const Matrix2& m, const Vector2& v) noexcept {
	return Vector2(
		v.x * m.m[0][0] + v.y * m.m[0][1],
		v.x * m.m[1][0] + v.y * m.m[1][1]
		);
}

Matrix2 operator * (const Matrix2& a, const Matrix2& b) noexcept; //!< matrix multiplication; result = a*b
Matrix2 inverseMatrix(const Matrix2& a) noexcept; //!< finds the inverse of a matrix (assuming it exists)
Matrix2 transpose(const Matrix2& a) noexcept; //!< finds the transposed matrix
float determinant(const Matrix2& a) noexcept; //!< finds the determinant of a matrix

Matrix2 rotationMatrix(const float angle) noexcept; //!< returns a ratation matrix with angle rotation (in Radians)

#endif // __MATRIX_H__
