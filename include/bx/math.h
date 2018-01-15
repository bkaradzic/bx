/*
 * Copyright 2011-2018 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

// FPU math lib

#ifndef BX_MATH_H_HEADER_GUARD
#define BX_MATH_H_HEADER_GUARD

#include "bx.h"
#include "uint32_t.h"

namespace bx
{
	extern const float kPi;
	extern const float kPi2;
	extern const float kInvPi;
	extern const float kPiHalf;
	extern const float kSqrt2;
	extern const float kInvLogNat2;
	extern const float kHuge;

	///
	typedef float (*LerpFn)(float _a, float _b, float _t);

	///
	struct Handness
	{
		enum Enum
		{
			Left,
			Right,
		};
	};

	///
	struct NearFar
	{
		enum Enum
		{
			Default,
			Reverse,
		};
	};

	///
	float toRad(float _deg);

	///
	float toDeg(float _rad);

	///
	uint32_t floatToBits(float _a);

	///
	float bitsToFloat(uint32_t _a);

	///
	uint64_t doubleToBits(double _a);

	///
	double bitsToDouble(uint64_t _a);

	///
	uint32_t floatFlip(uint32_t _value);

	///
	bool isNan(float _f);

	///
	bool isNan(double _f);

	///
	bool isFinite(float _f);

	///
	bool isFinite(double _f);

	///
	bool isInfinite(float _f);

	///
	bool isInfinite(double _f);

	///
	float floor(float _f);

	///
	float ceil(float _f);

	///
	float round(float _f);

	///
	float lerp(float _a, float _b, float _t);

	///
	float sign(float _a);

	///
	float abs(float _a);

	///
	float square(float _a);

	///
	float sin(float _a);

	///
	float asin(float _a);

	///
	float cos(float _a);

	///
	float tan(float _a);

	///
	float acos(float _a);

	///
	float atan2(float _y, float _x);

	///
	float pow(float _a, float _b);

	///
	float exp(float _a);

	///
	float exp2(float _a);

	///
	float log(float _a);

	///
	float log2(float _a);

	///
	float sqrt(float _a);

	///
	float rsqrt(float _a);

	///
	float trunc(float _a);

	///
	float fract(float _a);

	///
	float mod(float _a, float _b);

	///
	bool equal(float _a, float _b, float _epsilon);

	///
	bool equal(const float* _a, const float* _b, uint32_t _num, float _epsilon);

	///
	float wrap(float _a, float _wrap);

	///
	float step(float _edge, float _a);

	///
	float pulse(float _a, float _start, float _end);

	///
	float smoothStep(float _a);

	// References:
	//  - Bias And Gain Are Your Friend
	//    http://blog.demofox.org/2012/09/24/bias-and-gain-are-your-friend/
	//  - http://demofox.org/biasgain.html
	///
	float bias(float _time, float _bias);

	///
	float gain(float _time, float _gain);

	///
	float angleDiff(float _a, float _b);

	/// Shortest distance linear interpolation between two angles.
	float angleLerp(float _a, float _b, float _t);

	///
	void vec3Move(float* _result, const float* _a);

	///
	void vec3Abs(float* _result, const float* _a);

	///
	void vec3Neg(float* _result, const float* _a);

	///
	void vec3Add(float* _result, const float* _a, const float* _b);

	///
	void vec3Add(float* _result, const float* _a, float _b);

	///
	void vec3Sub(float* _result, const float* _a, const float* _b);

	///
	void vec3Sub(float* _result, const float* _a, float _b);

	///
	void vec3Mul(float* _result, const float* _a, const float* _b);

	///
	void vec3Mul(float* _result, const float* _a, float _b);

	///
	float vec3Dot(const float* _a, const float* _b);

	///
	void vec3Cross(float* _result, const float* _a, const float* _b);

	///
	float vec3Length(const float* _a);

	///
	void vec3Lerp(float* _result, const float* _a, const float* _b, float _t);

	///
	void vec3Lerp(float* _result, const float* _a, const float* _b, const float* _c);

	///
	float vec3Norm(float* _result, const float* _a);

	///
	void vec3Min(float* _result, const float* _a, const float* _b);

	///
	void vec3Max(float* _result, const float* _a, const float* _b);

	///
	void vec3Rcp(float* _result, const float* _a);

	/// Calculate tangent frame from normal.
	///
	void vec3TangentFrame(const float* _n, float* _t, float* _b);

	/// Calculate tangent frame from normal and angle.
	///
	void vec3TangentFrame(const float* _n, float* _t, float* _b, float _angle);

	///
	void vec3FromLatLong(float* _vec, float _u, float _v);

	/// Convert direction to 2D latitude and longitude.
	///
	/// @param[out] _outU U-coordinate.
	/// @param[out] _outV V-coordinate.
	/// @param[in]  _dir Normalized direction vector.
	///
	void vec3ToLatLong(float* _outU, float* _outV, const float* _dir);

	///
	void quatIdentity(float* _result);

	///
	void quatMove(float* _result, const float* _a);

	///
	void quatMulXYZ(float* _result, const float* _qa, const float* _qb);

	///
	void quatMul(float* _result, const float* _qa, const float* _qb);

	///
	void quatInvert(float* _result, const float* _quat);

	///
	float quatDot(const float* _a, const float* _b);

	///
	void quatNorm(float* _result, const float* _quat);

	///
	void quatToEuler(float* _result, const float* _quat);

	///
	void quatRotateAxis(float* _result, const float* _axis, float _angle);

	///
	void quatRotateX(float* _result, float _ax);

	///
	void quatRotateY(float* _result, float _ay);

	///
	void quatRotateZ(float* _result, float _az);

	///
	void vec3MulQuat(float* _result, const float* _vec, const float* _quat);

	///
	void mtxIdentity(float* _result);

	///
	void mtxTranslate(float* _result, float _tx, float _ty, float _tz);

	///
	void mtxScale(float* _result, float _sx, float _sy, float _sz);

	///
	void mtxScale(float* _result, float _scale);

	///
	void mtxFromNormal(float* _result, const float* _normal, float _scale, const float* _pos);

	///
	void mtxFromNormal(float* _result, const float* _normal, float _scale, const float* _pos, float _angle);

	///
	void mtxQuat(float* _result, const float* _quat);

	///
	void mtxQuatTranslation(float* _result, const float* _quat, const float* _translation);

	///
	void mtxQuatTranslationHMD(float* _result, const float* _quat, const float* _translation);

	///
	void mtxLookAtLh(float* _result, const float* _eye, const float* _at, const float* _up = NULL);

	///
	void mtxLookAtRh(float* _result, const float* _eye, const float* _at, const float* _up = NULL);

	///
	void mtxLookAt(float* _result, const float* _eye, const float* _at, const float* _up = NULL);

	///
	void mtxProj(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc);

	///
	void mtxProj(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc);

	///
	void mtxProj(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc);

	///
	void mtxProjLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc);

	///
	void mtxProjLh(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc);

	///
	void mtxProjLh(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc);

	///
	void mtxProjRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc);

	///
	void mtxProjRh(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc);

	///
	void mtxProjRh(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc);

	///
	void mtxProjInf(float* _result, const float _fov[4], float _near, bool _oglNdc);

	///
	void mtxProjInf(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc);

	///
	void mtxProjInf(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc);

	///
	void mtxProjInfLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc);

	///
	void mtxProjInfLh(float* _result, const float _fov[4], float _near, bool _oglNdc);

	///
	void mtxProjInfLh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc);

	///
	void mtxProjInfRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc);

	///
	void mtxProjInfRh(float* _result, const float _fov[4], float _near, bool _oglNdc);

	///
	void mtxProjInfRh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc);

	///
	void mtxProjRevInfLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc);

	///
	void mtxProjRevInfLh(float* _result, const float _fov[4], float _near, bool _oglNdc);

	///
	void mtxProjRevInfLh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc);

	///
	void mtxProjRevInfRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc);

	///
	void mtxProjRevInfRh(float* _result, const float _fov[4], float _near, bool _oglNdc);

	///
	void mtxProjRevInfRh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc);

	///
	void mtxOrtho(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc);

	///
	void mtxOrthoLh(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc);

	///
	void mtxOrthoRh(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc);

	///
	void mtxRotateX(float* _result, float _ax);

	///
	void mtxRotateY(float* _result, float _ay);

	///
	void mtxRotateZ(float* _result, float _az);

	///
	void mtxRotateXY(float* _result, float _ax, float _ay);

	///
	void mtxRotateXYZ(float* _result, float _ax, float _ay, float _az);

	///
	void mtxRotateZYX(float* _result, float _ax, float _ay, float _az);

	///
	void mtxSRT(float* _result, float _sx, float _sy, float _sz, float _ax, float _ay, float _az, float _tx, float _ty, float _tz);

	///
	void vec3MulMtx(float* _result, const float* _vec, const float* _mat);

	///
	void vec3MulMtxXyz0(float* _result, const float* _vec, const float* _mat);

	///
	void vec3MulMtxH(float* _result, const float* _vec, const float* _mat);

	///
	void vec4Mul(float* _result, const float* _a, const float* _b);

	///
	void vec4Mul(float* _result, const float* _a, float _b);

	///
	void vec4MulMtx(float* _result, const float* _vec, const float* _mat);

	///
	void mtxMul(float* _result, const float* _a, const float* _b);

	///
	void mtxTranspose(float* _result, const float* _a);

	///
	void mtx3Inverse(float* _result, const float* _a);

	///
	void mtxInverse(float* _result, const float* _a);

	/// Convert LH to RH projection matrix and vice versa.
	///
	void mtxProjFlipHandedness(float* _dst, const float* _src);

	/// Convert LH to RH view matrix and vice versa.
	///
	void mtxViewFlipHandedness(float* _dst, const float* _src);

	///
	void calcNormal(float _result[3], float _va[3], float _vb[3], float _vc[3]);

	///
	void calcPlane(float _result[4], float _va[3], float _vb[3], float _vc[3]);

	///
	void calcLinearFit2D(float _result[2], const void* _points, uint32_t _stride, uint32_t _numPoints);

	///
	void calcLinearFit3D(float _result[3], const void* _points, uint32_t _stride, uint32_t _numPoints);

	///
	void rgbToHsv(float _hsv[3], const float _rgb[3]);

	///
	void hsvToRgb(float _rgb[3], const float _hsv[3]);

} // namespace bx

#include "inline/math.inl"

#endif // BX_MATH_H_HEADER_GUARD
