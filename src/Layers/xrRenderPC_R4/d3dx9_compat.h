// Minimal replacement for the proprietary D3DX9 utility library, which
// dxvk-native does not implement (it only provides core D3D9/DXGI device
// interfaces) and for which there is no Linux runtime to link against.
//
// This only implements the subset of D3DX9 actually exercised by code that
// is compiled into this port's DX11 renderer target: 3D math (vectors,
// matrices, planes) and the D3D9 vertex-declaration/FVF utilities. The
// texture-IO and shader-bytecode-reflection portions of the real D3DX9 API
// (D3DXCreateTexture*, D3DXSaveTextureTo*, D3DXCompileShader,
// D3DXSHADER_CONSTANTTABLE, ...) are deliberately not provided here: every
// call site that used them lives in a `#if !defined(USE_DX10) &&
// !defined(USE_DX11)` (or `#ifdef _EDITOR`) branch that this target does not
// compile.
#ifndef D3DX9_COMPAT_H
#define D3DX9_COMPAT_H
#pragma once

#include <cmath>
#include <cstring>
#include <cstdint>

// Real D3DX9 headers always transitively pull in <d3d9.h> (the vertex-
// declaration/FVF utilities below operate on D3DVERTEXELEMENT9 and other
// plain D3D9 types, not D3DX-specific ones); dxvk-native ships real D3D9
// headers, unlike D3DX itself which it doesn't implement.
#include <d3d9.h>

#define D3DX_DEFAULT ((UINT)-1)

// classic real D3DX9 values
#define D3DXSHADER_DEBUG 0x1
#define D3DXSHADER_PACKMATRIX_ROWMAJOR 0x8
#define D3DXSHADER_PREFER_FLOW_CONTROL 0x40000

//-----------------------------------------------------------------------------
// Vectors / matrix / plane
//-----------------------------------------------------------------------------
struct D3DXVECTOR2
{
	float x, y;

	D3DXVECTOR2() {}
	D3DXVECTOR2(float _x, float _y) : x(_x), y(_y) {}
	explicit D3DXVECTOR2(const float* p) : x(p[0]), y(p[1]) {}

	operator float*() { return &x; }
	operator const float*() const { return &x; }

	D3DXVECTOR2& operator+=(const D3DXVECTOR2& v) { x += v.x; y += v.y; return *this; }
	D3DXVECTOR2& operator-=(const D3DXVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
	D3DXVECTOR2& operator*=(float s) { x *= s; y *= s; return *this; }
	D3DXVECTOR2& operator/=(float s) { x /= s; y /= s; return *this; }

	D3DXVECTOR2 operator+() const { return *this; }
	D3DXVECTOR2 operator-() const { return D3DXVECTOR2(-x, -y); }

	D3DXVECTOR2 operator+(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x + v.x, y + v.y); }
	D3DXVECTOR2 operator-(const D3DXVECTOR2& v) const { return D3DXVECTOR2(x - v.x, y - v.y); }
	D3DXVECTOR2 operator*(float s) const { return D3DXVECTOR2(x * s, y * s); }
	D3DXVECTOR2 operator/(float s) const { return D3DXVECTOR2(x / s, y / s); }
	friend D3DXVECTOR2 operator*(float s, const D3DXVECTOR2& v) { return D3DXVECTOR2(v.x * s, v.y * s); }

	bool operator==(const D3DXVECTOR2& v) const { return x == v.x && y == v.y; }
	bool operator!=(const D3DXVECTOR2& v) const { return !(*this == v); }
};

struct D3DXVECTOR3
{
	float x, y, z;

	D3DXVECTOR3() {}
	D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	explicit D3DXVECTOR3(const float* p) : x(p[0]), y(p[1]), z(p[2]) {}

	operator float*() { return &x; }
	operator const float*() const { return &x; }

	D3DXVECTOR3& operator+=(const D3DXVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	D3DXVECTOR3& operator-=(const D3DXVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	D3DXVECTOR3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
	D3DXVECTOR3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

	D3DXVECTOR3 operator+() const { return *this; }
	D3DXVECTOR3 operator-() const { return D3DXVECTOR3(-x, -y, -z); }

	D3DXVECTOR3 operator+(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
	D3DXVECTOR3 operator-(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
	D3DXVECTOR3 operator*(float s) const { return D3DXVECTOR3(x * s, y * s, z * s); }
	D3DXVECTOR3 operator/(float s) const { return D3DXVECTOR3(x / s, y / s, z / s); }
	friend D3DXVECTOR3 operator*(float s, const D3DXVECTOR3& v) { return D3DXVECTOR3(v.x * s, v.y * s, v.z * s); }

	bool operator==(const D3DXVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const D3DXVECTOR3& v) const { return !(*this == v); }
};

struct D3DXVECTOR4
{
	float x, y, z, w;

	D3DXVECTOR4() {}
	D3DXVECTOR4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	explicit D3DXVECTOR4(const float* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}

	operator float*() { return &x; }
	operator const float*() const { return &x; }

	D3DXVECTOR4& operator+=(const D3DXVECTOR4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	D3DXVECTOR4& operator-=(const D3DXVECTOR4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	D3DXVECTOR4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	D3DXVECTOR4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

	D3DXVECTOR4 operator+() const { return *this; }
	D3DXVECTOR4 operator-() const { return D3DXVECTOR4(-x, -y, -z, -w); }

	D3DXVECTOR4 operator+(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
	D3DXVECTOR4 operator-(const D3DXVECTOR4& v) const { return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
	D3DXVECTOR4 operator*(float s) const { return D3DXVECTOR4(x * s, y * s, z * s, w * s); }
	D3DXVECTOR4 operator/(float s) const { return D3DXVECTOR4(x / s, y / s, z / s, w / s); }
	friend D3DXVECTOR4 operator*(float s, const D3DXVECTOR4& v) { return D3DXVECTOR4(v.x * s, v.y * s, v.z * s, v.w * s); }

	bool operator==(const D3DXVECTOR4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool operator!=(const D3DXVECTOR4& v) const { return !(*this == v); }
};

// Row-major, same field layout/order as xrCore's Fmatrix - the two are
// freely reinterpret_cast<>'d between at call sites in the renderer.
struct D3DXMATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};

	D3DXMATRIX() {}

	D3DXMATRIX(float m11, float m12, float m13, float m14, float m21, float m22, float m23, float m24, float m31,
	           float m32, float m33, float m34, float m41, float m42, float m43, float m44)
	    : _11(m11), _12(m12), _13(m13), _14(m14), _21(m21), _22(m22), _23(m23), _24(m24), _31(m31), _32(m32),
	      _33(m33), _34(m34), _41(m41), _42(m42), _43(m43), _44(m44)
	{
	}

	float& operator()(int r, int c) { return m[r][c]; }
	float operator()(int r, int c) const { return m[r][c]; }

	operator float*() { return &_11; }
	operator const float*() const { return &_11; }

	D3DXMATRIX operator*(const D3DXMATRIX& b) const
	{
		D3DXMATRIX r;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				r.m[i][j] = m[i][0] * b.m[0][j] + m[i][1] * b.m[1][j] + m[i][2] * b.m[2][j] + m[i][3] * b.m[3][j];
		return r;
	}
	D3DXMATRIX& operator*=(const D3DXMATRIX& b) { *this = *this * b; return *this; }
};

struct D3DXPLANE
{
	float a, b, c, d;

	D3DXPLANE() {}
	D3DXPLANE(float _a, float _b, float _c, float _d) : a(_a), b(_b), c(_c), d(_d) {}
	explicit D3DXPLANE(const float* p) : a(p[0]), b(p[1]), c(p[2]), d(p[3]) {}

	operator float*() { return &a; }
	operator const float*() const { return &a; }
};

typedef uint16_t D3DXFLOAT16;

//-----------------------------------------------------------------------------
// Matrix functions
//-----------------------------------------------------------------------------
inline D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
	std::memset(pOut, 0, sizeof(D3DXMATRIX));
	pOut->_11 = pOut->_22 = pOut->_33 = pOut->_44 = 1.f;
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2)
{
	*pOut = (*pM1) * (*pM2);
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
	D3DXMATRIX r;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			r.m[i][j] = pM->m[j][i];
	*pOut = r;
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* pOut, float x, float y, float z)
{
	D3DXMatrixIdentity(pOut);
	pOut->_41 = x; pOut->_42 = y; pOut->_43 = z;
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* pOut, float x, float y, float z)
{
	D3DXMatrixIdentity(pOut);
	pOut->_11 = x; pOut->_22 = y; pOut->_33 = z;
	return pOut;
}

inline D3DXMATRIX* D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, float l, float r, float b, float t, float zn, float zf)
{
	D3DXMatrixIdentity(pOut);
	pOut->_11 = 2.f / (r - l);
	pOut->_22 = 2.f / (t - b);
	pOut->_33 = 1.f / (zf - zn);
	pOut->_41 = -(l + r) / (r - l);
	pOut->_42 = -(t + b) / (t - b);
	pOut->_43 = -zn / (zf - zn);
	return pOut;
}

// General 4x4 inverse via cofactor expansion; pDeterminant is filled in if
// non-null (no call site in this codebase actually reads it).
inline D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM)
{
	const float* s = &pM->_11;
	float inv[16];

	inv[0] = s[5] * s[10] * s[15] - s[5] * s[11] * s[14] - s[9] * s[6] * s[15] + s[9] * s[7] * s[14] + s[13] * s[6] * s[11] - s[13] * s[7] * s[10];
	inv[4] = -s[4] * s[10] * s[15] + s[4] * s[11] * s[14] + s[8] * s[6] * s[15] - s[8] * s[7] * s[14] - s[12] * s[6] * s[11] + s[12] * s[7] * s[10];
	inv[8] = s[4] * s[9] * s[15] - s[4] * s[11] * s[13] - s[8] * s[5] * s[15] + s[8] * s[7] * s[13] + s[12] * s[5] * s[11] - s[12] * s[7] * s[9];
	inv[12] = -s[4] * s[9] * s[14] + s[4] * s[10] * s[13] + s[8] * s[5] * s[14] - s[8] * s[6] * s[13] - s[12] * s[5] * s[10] + s[12] * s[6] * s[9];

	inv[1] = -s[1] * s[10] * s[15] + s[1] * s[11] * s[14] + s[9] * s[2] * s[15] - s[9] * s[3] * s[14] - s[13] * s[2] * s[11] + s[13] * s[3] * s[10];
	inv[5] = s[0] * s[10] * s[15] - s[0] * s[11] * s[14] - s[8] * s[2] * s[15] + s[8] * s[3] * s[14] + s[12] * s[2] * s[11] - s[12] * s[3] * s[10];
	inv[9] = -s[0] * s[9] * s[15] + s[0] * s[11] * s[13] + s[8] * s[1] * s[15] - s[8] * s[3] * s[13] - s[12] * s[1] * s[11] + s[12] * s[3] * s[9];
	inv[13] = s[0] * s[9] * s[14] - s[0] * s[10] * s[13] - s[8] * s[1] * s[14] + s[8] * s[2] * s[13] + s[12] * s[1] * s[10] - s[12] * s[2] * s[9];

	inv[2] = s[1] * s[6] * s[15] - s[1] * s[7] * s[14] - s[5] * s[2] * s[15] + s[5] * s[3] * s[14] + s[13] * s[2] * s[7] - s[13] * s[3] * s[6];
	inv[6] = -s[0] * s[6] * s[15] + s[0] * s[7] * s[14] + s[4] * s[2] * s[15] - s[4] * s[3] * s[14] - s[12] * s[2] * s[7] + s[12] * s[3] * s[6];
	inv[10] = s[0] * s[5] * s[15] - s[0] * s[7] * s[13] - s[4] * s[1] * s[15] + s[4] * s[3] * s[13] + s[12] * s[1] * s[7] - s[12] * s[3] * s[5];
	inv[14] = -s[0] * s[5] * s[14] + s[0] * s[6] * s[13] + s[4] * s[1] * s[14] - s[4] * s[2] * s[13] - s[12] * s[1] * s[6] + s[12] * s[2] * s[5];

	inv[3] = -s[1] * s[6] * s[11] + s[1] * s[7] * s[10] + s[5] * s[2] * s[11] - s[5] * s[3] * s[10] - s[9] * s[2] * s[7] + s[9] * s[3] * s[6];
	inv[7] = s[0] * s[6] * s[11] - s[0] * s[7] * s[10] - s[4] * s[2] * s[11] + s[4] * s[3] * s[10] + s[8] * s[2] * s[7] - s[8] * s[3] * s[6];
	inv[11] = -s[0] * s[5] * s[11] + s[0] * s[7] * s[9] + s[4] * s[1] * s[11] - s[4] * s[3] * s[9] - s[8] * s[1] * s[7] + s[8] * s[3] * s[5];
	inv[15] = s[0] * s[5] * s[10] - s[0] * s[6] * s[9] - s[4] * s[1] * s[10] + s[4] * s[2] * s[9] + s[8] * s[1] * s[6] - s[8] * s[2] * s[5];

	float det = s[0] * inv[0] + s[1] * inv[4] + s[2] * inv[8] + s[3] * inv[12];
	if (pDeterminant) *pDeterminant = det;
	if (det == 0.f) return nullptr;

	float invDet = 1.f / det;
	float* o = &pOut->_11;
	for (int i = 0; i < 16; ++i) o[i] = inv[i] * invDet;
	return pOut;
}

//-----------------------------------------------------------------------------
// Vector functions
//-----------------------------------------------------------------------------
inline float D3DXVec2Dot(const D3DXVECTOR2* v1, const D3DXVECTOR2* v2) { return v1->x * v2->x + v1->y * v2->y; }
inline float D3DXVec2Length(const D3DXVECTOR2* v) { return std::sqrt(v->x * v->x + v->y * v->y); }

inline float D3DXVec3Dot(const D3DXVECTOR3* v1, const D3DXVECTOR3* v2) { return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z; }
inline float D3DXVec3Length(const D3DXVECTOR3* v) { return std::sqrt(D3DXVec3Dot(v, v)); }

inline D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
	D3DXVECTOR3 r(
		pV1->y * pV2->z - pV1->z * pV2->y,
		pV1->z * pV2->x - pV1->x * pV2->z,
		pV1->x * pV2->y - pV1->y * pV2->x);
	*pOut = r;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
	float len = D3DXVec3Length(pV);
	if (len == 0.f) { *pOut = *pV; return pOut; }
	*pOut = *pV / len;
	return pOut;
}

inline D3DXVECTOR4* D3DXVec3Transform(D3DXVECTOR4* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	D3DXVECTOR4 r(
		pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pM->_41,
		pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pM->_42,
		pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pM->_43,
		pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pM->_44);
	*pOut = r;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	float x = pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31 + pM->_41;
	float y = pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32 + pM->_42;
	float z = pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33 + pM->_43;
	float w = pV->x * pM->_14 + pV->y * pM->_24 + pV->z * pM->_34 + pM->_44;
	if (w == 0.f) w = 1.f;
	pOut->x = x / w; pOut->y = y / w; pOut->z = z / w;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV, const D3DXMATRIX* pM)
{
	D3DXVECTOR3 r(
		pV->x * pM->_11 + pV->y * pM->_21 + pV->z * pM->_31,
		pV->x * pM->_12 + pV->y * pM->_22 + pV->z * pM->_32,
		pV->x * pM->_13 + pV->y * pM->_23 + pV->z * pM->_33);
	*pOut = r;
	return pOut;
}

inline D3DXVECTOR3* D3DXVec3TransformCoordArray(D3DXVECTOR3* pOut, unsigned int outStride, const D3DXVECTOR3* pV,
	unsigned int inStride, const D3DXMATRIX* pM, unsigned int n)
{
	const unsigned char* src = reinterpret_cast<const unsigned char*>(pV);
	unsigned char* dst = reinterpret_cast<unsigned char*>(pOut);
	for (unsigned int i = 0; i < n; ++i)
	{
		D3DXVECTOR3 tmp;
		D3DXVec3TransformCoord(&tmp, reinterpret_cast<const D3DXVECTOR3*>(src), pM);
		*reinterpret_cast<D3DXVECTOR3*>(dst) = tmp;
		src += inStride;
		dst += outStride;
	}
	return pOut;
}

//-----------------------------------------------------------------------------
// Plane functions
//-----------------------------------------------------------------------------
inline float D3DXPlaneDotCoord(const D3DXPLANE* pP, const D3DXVECTOR3* pV)
{
	return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

inline float D3DXPlaneDotNormal(const D3DXPLANE* pP, const D3DXVECTOR3* pV)
{
	return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z;
}

inline D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP)
{
	float len = std::sqrt(pP->a * pP->a + pP->b * pP->b + pP->c * pP->c);
	if (len == 0.f) { *pOut = *pP; return pOut; }
	float invLen = 1.f / len;
	pOut->a = pP->a * invLen; pOut->b = pP->b * invLen; pOut->c = pP->c * invLen; pOut->d = pP->d * invLen;
	return pOut;
}

inline D3DXPLANE* D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM)
{
	D3DXPLANE r(
		pP->a * pM->_11 + pP->b * pM->_21 + pP->c * pM->_31 + pP->d * pM->_41,
		pP->a * pM->_12 + pP->b * pM->_22 + pP->c * pM->_32 + pP->d * pM->_42,
		pP->a * pM->_13 + pP->b * pM->_23 + pP->c * pM->_33 + pP->d * pM->_43,
		pP->a * pM->_14 + pP->b * pM->_24 + pP->c * pM->_34 + pP->d * pM->_44);
	*pOut = r;
	return pOut;
}

//-----------------------------------------------------------------------------
// Half-float conversion (IEEE binary16 <-> binary32)
//-----------------------------------------------------------------------------
inline float D3DXFloat16to32(D3DXFLOAT16 h)
{
	uint32_t sign = (h & 0x8000u) << 16;
	uint32_t exp = (h >> 10) & 0x1Fu;
	uint32_t mant = h & 0x3FFu;
	uint32_t bits;
	if (exp == 0)
	{
		if (mant == 0) { bits = sign; }
		else
		{
			// subnormal half -> normalized float
			int e = -1;
			do { mant <<= 1; ++e; } while (!(mant & 0x400u));
			mant &= 0x3FFu;
			bits = sign | ((127 - 15 - e) << 23) | (mant << 13);
		}
	}
	else if (exp == 31)
	{
		bits = sign | 0x7F800000u | (mant << 13);
	}
	else
	{
		bits = sign | ((exp - 15 + 127) << 23) | (mant << 13);
	}
	float f;
	std::memcpy(&f, &bits, sizeof(f));
	return f;
}

inline D3DXFLOAT16 D3DXFloat32to16(float f)
{
	uint32_t bits;
	std::memcpy(&bits, &f, sizeof(bits));
	uint32_t sign = (bits >> 16) & 0x8000u;
	int32_t exp = int32_t((bits >> 23) & 0xFF) - 127 + 15;
	uint32_t mant = bits & 0x7FFFFFu;

	if (exp <= 0)
	{
		// too small -> flush to signed zero (adequate for this port's usage)
		return D3DXFLOAT16(sign);
	}
	if (exp >= 31)
	{
		// overflow/inf/nan -> inf
		return D3DXFLOAT16(sign | 0x7C00u);
	}
	return D3DXFLOAT16(sign | (uint32_t(exp) << 10) | (mant >> 13));
}

inline float* D3DXFloat16To32Array(float* pOut, const D3DXFLOAT16* pIn, unsigned int n)
{
	for (unsigned int i = 0; i < n; ++i) pOut[i] = D3DXFloat16to32(pIn[i]);
	return pOut;
}

inline D3DXFLOAT16* D3DXFloat32To16Array(D3DXFLOAT16* pOut, const float* pIn, unsigned int n)
{
	for (unsigned int i = 0; i < n; ++i) pOut[i] = D3DXFloat32to16(pIn[i]);
	return pOut;
}

//-----------------------------------------------------------------------------
// D3D9 vertex declaration / FVF utilities
//-----------------------------------------------------------------------------
#ifndef MAX_FVF_DECL_SIZE
#define MAX_FVF_DECL_SIZE 20
#endif

inline unsigned int D3DXGetDeclLength(const D3DVERTEXELEMENT9* pDecl)
{
	unsigned int n = 0;
	while (pDecl[n].Stream != 0xFF) ++n;
	return n;
}

inline unsigned int d3dx9_compat_decl_type_size(BYTE type)
{
	switch (type)
	{
	case D3DDECLTYPE_FLOAT1: return 4;
	case D3DDECLTYPE_FLOAT2: return 8;
	case D3DDECLTYPE_FLOAT3: return 12;
	case D3DDECLTYPE_FLOAT4: return 16;
	case D3DDECLTYPE_D3DCOLOR: return 4;
	case D3DDECLTYPE_UBYTE4: return 4;
	case D3DDECLTYPE_SHORT2: return 4;
	case D3DDECLTYPE_SHORT4: return 8;
	case D3DDECLTYPE_UBYTE4N: return 4;
	case D3DDECLTYPE_SHORT2N: return 4;
	case D3DDECLTYPE_SHORT4N: return 8;
	case D3DDECLTYPE_USHORT2N: return 4;
	case D3DDECLTYPE_USHORT4N: return 8;
	case D3DDECLTYPE_UDEC3: return 4;
	case D3DDECLTYPE_DEC3N: return 4;
	case D3DDECLTYPE_FLOAT16_2: return 4;
	case D3DDECLTYPE_FLOAT16_4: return 8;
	default: return 0;
	}
}

inline unsigned int D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9* pDecl, unsigned int stream)
{
	unsigned int size = 0;
	for (unsigned int i = 0; pDecl[i].Stream != 0xFF; ++i)
	{
		if (pDecl[i].Stream != stream) continue;
		unsigned int end = pDecl[i].Offset + d3dx9_compat_decl_type_size(pDecl[i].Type);
		if (end > size) size = end;
	}
	return size;
}

inline unsigned int D3DXGetFVFVertexSize(DWORD FVF)
{
	unsigned int size = 0;

	switch (FVF & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ: size += 3 * 4; break;
	case D3DFVF_XYZRHW: size += 4 * 4; break;
	case D3DFVF_XYZB1: size += 4 * 4; break;
	case D3DFVF_XYZB2: size += 5 * 4; break;
	case D3DFVF_XYZB3: size += 6 * 4; break;
	case D3DFVF_XYZB4: size += 7 * 4; break;
	case D3DFVF_XYZB5: size += 8 * 4; break;
	case D3DFVF_XYZW: size += 4 * 4; break;
	default: break;
	}

	if (FVF & D3DFVF_NORMAL) size += 3 * 4;
	if (FVF & D3DFVF_PSIZE) size += 4;
	if (FVF & D3DFVF_DIFFUSE) size += 4;
	if (FVF & D3DFVF_SPECULAR) size += 4;

	unsigned int texCount = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for (unsigned int i = 0; i < texCount; ++i)
	{
		unsigned int fmt = (FVF >> (16 + i * 2)) & 0x3;
		switch (fmt)
		{
		case D3DFVF_TEXTUREFORMAT1: size += 1 * 4; break;
		case D3DFVF_TEXTUREFORMAT2: size += 2 * 4; break;
		case D3DFVF_TEXTUREFORMAT3: size += 3 * 4; break;
		case D3DFVF_TEXTUREFORMAT4: size += 4 * 4; break;
		}
	}

	return size;
}

inline HRESULT D3DXDeclaratorFromFVF(DWORD FVF, D3DVERTEXELEMENT9 decl[/*MAXD3DDECLLENGTH*/])
{
	unsigned int idx = 0;
	unsigned int offset = 0;

	switch (FVF & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ:
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 };
		offset += 12;
		break;
	case D3DFVF_XYZRHW:
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 };
		offset += 16;
		break;
	default:
		// blended-position FVFs (XYZBn) aren't used anywhere in this port's
		// compiled sources; fall back to plain XYZ if one ever shows up.
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 };
		offset += 12;
		break;
	}

	if (FVF & D3DFVF_NORMAL)
	{
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 };
		offset += 12;
	}
	if (FVF & D3DFVF_DIFFUSE)
	{
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 };
		offset += 4;
	}
	if (FVF & D3DFVF_SPECULAR)
	{
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1 };
		offset += 4;
	}

	unsigned int texCount = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for (unsigned int i = 0; i < texCount; ++i)
	{
		unsigned int fmt = (FVF >> (16 + i * 2)) & 0x3;
		BYTE type; unsigned int sz;
		switch (fmt)
		{
		case D3DFVF_TEXTUREFORMAT1: type = D3DDECLTYPE_FLOAT1; sz = 4; break;
		case D3DFVF_TEXTUREFORMAT3: type = D3DDECLTYPE_FLOAT3; sz = 12; break;
		case D3DFVF_TEXTUREFORMAT4: type = D3DDECLTYPE_FLOAT4; sz = 16; break;
		case D3DFVF_TEXTUREFORMAT2:
		default: type = D3DDECLTYPE_FLOAT2; sz = 8; break;
		}
		decl[idx++] = D3DVERTEXELEMENT9{ 0, WORD(offset), type, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, BYTE(i) };
		offset += sz;
	}

	decl[idx] = D3DVERTEXELEMENT9{ 0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0 };
	return S_OK;
}

#endif // D3DX9_COMPAT_H
