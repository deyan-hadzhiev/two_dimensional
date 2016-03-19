#ifndef __MATRIX_H__
#define __MATRIX_H__

#include "vector2.h"

__declspec( align(16) )
class Matrix4 {
public:
	union	{
		float ma[16];
		float m[4][4];
		Vector4 rows[4];
	};

	Matrix4() noexcept {}
	Matrix4(float diagonalElement) noexcept
		: ma{ 0 }
	{
		m[0][0] = m[1][1] = m[2][2] = m[3][3] = diagonalElement;
	}

	Matrix4(const float arr[16]) noexcept
		: ma{
		arr[0x0], arr[0x1], arr[0x2], arr[0x3],
		arr[0x4], arr[0x5], arr[0x6], arr[0x7],
		arr[0x8], arr[0x9], arr[0xa], arr[0xb],
		arr[0xc], arr[0xd], arr[0xe], arr[0xf]
	}	{}

	Matrix4(
		float ax0, float ax1, float ax2, float ax3,
		float ax4, float ax5, float ax6, float ax7,
		float ax8, float ax9, float axa, float axb,
		float axc, float axd, float axe, float axf
		) noexcept
		: ma{
		ax0, ax1, ax2, ax3,
		ax4, ax5, ax6, ax7,
		ax8, ax9, axa, axb,
		axc, axd, axe, axf
	}
	{}
};

// TODO: Some day optimize with some intrinsic calls - but lets have some basic stuff for the begining
inline Vector4 operator * (const Vector4& v, const Matrix4& m) noexcept {
	return Vector4(
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
		v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3]
		);
}

Matrix4 operator * (const Matrix4& a, const Matrix4& b) noexcept; //!< matrix multiplication; result = a*b
Matrix4 inverseMatrix(const Matrix4& a) noexcept; //!< finds the inverse of a matrix (assuming it exists)
Matrix4 transpose(const Matrix4& a) noexcept; //!< finds the transposed matrix
float determinant(const Matrix4& a) noexcept; //!< finds the determinant of a matrix

// Rotation matrices in four dimensions is rotatation around a plane - two axis
Matrix4 rotationAroundXY(float angle) noexcept; //!< returns a rotation matrix around the XY plane; the angle is in radians
Matrix4 rotationAroundYZ(float angle) noexcept; //!< same as above, but rotate around YZ
Matrix4 rotationAroundZX(float angle) noexcept; //!< same as above, but rotate around ZX
Matrix4 rotationAroundXW(float angle) noexcept; //!< same as above, but rotate around XW
Matrix4 rotationAroundYW(float angle) noexcept; //!< same as above, but rotate around YW
Matrix4 rotationAroundZW(float angle) noexcept; //!< same as above, but rotate around ZW

#endif // __MATRIX_H__
