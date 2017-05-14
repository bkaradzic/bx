/*
 * Copyright 2011-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

// FPU math lib

#ifndef BX_FPU_MATH_H_HEADER_GUARD
#	error "Must be included from bx/fpumath.h!"
#endif // BX_FPU_MATH_H_HEADER_GUARD

namespace bx
{
	inline float toRad(float _deg)
	{
		return _deg * pi / 180.0f;
	}

	inline float toDeg(float _rad)
	{
		return _rad * 180.0f / pi;
	}

	inline uint32_t floatToBits(float _a)
	{
		union { float f; uint32_t ui; } u = { _a };
		return u.ui;
	}

	inline float bitsToFloat(uint32_t _a)
	{
		union { uint32_t ui; float f; } u = { _a };
		return u.f;
	}

	inline uint64_t doubleToBits(double _a)
	{
		union { double f; uint64_t ui; } u = { _a };
		return u.ui;
	}

	inline double bitsToDouble(uint64_t _a)
	{
		union { uint64_t ui; double f; } u = { _a };
		return u.f;
	}

	inline bool isNan(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp > UINT32_C(0x7f800000);
	}

	inline bool isNan(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp > UINT64_C(0x7ff0000000000000);
	}

	inline bool isFinite(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp < UINT32_C(0x7f800000);
	}

	inline bool isFinite(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp < UINT64_C(0x7ff0000000000000);
	}

	inline bool isInfinite(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp == UINT32_C(0x7f800000);
	}

	inline bool isInfinite(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp == UINT64_C(0x7ff0000000000000);
	}

	inline float fround(float _f)
	{
		return ffloor(_f + 0.5f);
	}

	inline float fmin(float _a, float _b)
	{
		return _a < _b ? _a : _b;
	}

	inline float fmax(float _a, float _b)
	{
		return _a > _b ? _a : _b;
	}

	inline float fmin3(float _a, float _b, float _c)
	{
		return fmin(_a, fmin(_b, _c) );
	}

	inline float fmax3(float _a, float _b, float _c)
	{
		return fmax(_a, fmax(_b, _c) );
	}

	inline float fclamp(float _a, float _min, float _max)
	{
		return fmin(fmax(_a, _min), _max);
	}

	inline float fsaturate(float _a)
	{
		return fclamp(_a, 0.0f, 1.0f);
	}

	inline float flerp(float _a, float _b, float _t)
	{
		return _a + (_b - _a) * _t;
	}

	inline float fsign(float _a)
	{
		return _a < 0.0f ? -1.0f : 1.0f;
	}

	inline float fsq(float _a)
	{
		return _a * _a;
	}

	inline float fexp2(float _a)
	{
		return fpow(2.0f, _a);
	}

	inline float frsqrt(float _a)
	{
		return 1.0f/fsqrt(_a);
	}

	inline float ffract(float _a)
	{
		return _a - ffloor(_a);
	}

	inline bool fequal(float _a, float _b, float _epsilon)
	{
		// http://realtimecollisiondetection.net/blog/?p=89
		const float lhs = fabsolute(_a - _b);
		const float rhs = _epsilon * fmax3(1.0f, fabsolute(_a), fabsolute(_b) );
		return lhs <= rhs;
	}

	inline bool fequal(const float* _a, const float* _b, uint32_t _num, float _epsilon)
	{
		bool equal = fequal(_a[0], _b[0], _epsilon);
		for (uint32_t ii = 1; equal && ii < _num; ++ii)
		{
			equal = fequal(_a[ii], _b[ii], _epsilon);
		}
		return equal;
	}

	inline float fwrap(float _a, float _wrap)
	{
		const float mod    = fmod(_a, _wrap);
		const float result = mod < 0.0f ? _wrap + mod : mod;
		return result;
	}

	inline float fstep(float _edge, float _a)
	{
		return _a < _edge ? 0.0f : 1.0f;
	}

	inline float fpulse(float _a, float _start, float _end)
	{
		return fstep(_a, _start) - fstep(_a, _end);
	}

	inline float fsmoothstep(float _a)
	{
		return fsq(_a)*(3.0f - 2.0f*_a);
	}

	inline float fbias(float _time, float _bias)
	{
		return _time / ( ( (1.0f/_bias - 2.0f)*(1.0f - _time) ) + 1.0f);
	}

	inline float fgain(float _time, float _gain)
	{
		if (_time < 0.5f)
		{
			return fbias(_time * 2.0f, _gain) * 0.5f;
		}

		return fbias(_time * 2.0f - 1.0f, 1.0f - _gain) * 0.5f + 0.5f;
	}

	inline void vec3Move(float* _result, const float* _a)
	{
		_result[0] = _a[0];
		_result[1] = _a[1];
		_result[2] = _a[2];
	}

	inline void vec3Abs(float* _result, const float* _a)
	{
		_result[0] = fabsolute(_a[0]);
		_result[1] = fabsolute(_a[1]);
		_result[2] = fabsolute(_a[2]);
	}

	inline void vec3Neg(float* _result, const float* _a)
	{
		_result[0] = -_a[0];
		_result[1] = -_a[1];
		_result[2] = -_a[2];
	}

	inline void vec3Add(float* _result, const float* _a, const float* _b)
	{
		_result[0] = _a[0] + _b[0];
		_result[1] = _a[1] + _b[1];
		_result[2] = _a[2] + _b[2];
	}

	inline void vec3Add(float* _result, const float* _a, float _b)
	{
		_result[0] = _a[0] + _b;
		_result[1] = _a[1] + _b;
		_result[2] = _a[2] + _b;
	}

	inline void vec3Sub(float* _result, const float* _a, const float* _b)
	{
		_result[0] = _a[0] - _b[0];
		_result[1] = _a[1] - _b[1];
		_result[2] = _a[2] - _b[2];
	}

	inline void vec3Sub(float* _result, const float* _a, float _b)
	{
		_result[0] = _a[0] - _b;
		_result[1] = _a[1] - _b;
		_result[2] = _a[2] - _b;
	}

	inline void vec3Mul(float* _result, const float* _a, const float* _b)
	{
		_result[0] = _a[0] * _b[0];
		_result[1] = _a[1] * _b[1];
		_result[2] = _a[2] * _b[2];
	}

	inline void vec3Mul(float* _result, const float* _a, float _b)
	{
		_result[0] = _a[0] * _b;
		_result[1] = _a[1] * _b;
		_result[2] = _a[2] * _b;
	}

	inline float vec3Dot(const float* _a, const float* _b)
	{
		return _a[0]*_b[0] + _a[1]*_b[1] + _a[2]*_b[2];
	}

	inline void vec3Cross(float* _result, const float* _a, const float* _b)
	{
		_result[0] = _a[1]*_b[2] - _a[2]*_b[1];
		_result[1] = _a[2]*_b[0] - _a[0]*_b[2];
		_result[2] = _a[0]*_b[1] - _a[1]*_b[0];
	}

	inline float vec3Length(const float* _a)
	{
		return fsqrt(vec3Dot(_a, _a) );
	}

	inline void vec3Lerp(float* _result, const float* _a, const float* _b, float _t)
	{
		_result[0] = flerp(_a[0], _b[0], _t);
		_result[1] = flerp(_a[1], _b[1], _t);
		_result[2] = flerp(_a[2], _b[2], _t);
	}

	inline void vec3Lerp(float* _result, const float* _a, const float* _b, const float* _c)
	{
		_result[0] = flerp(_a[0], _b[0], _c[0]);
		_result[1] = flerp(_a[1], _b[1], _c[1]);
		_result[2] = flerp(_a[2], _b[2], _c[2]);
	}

	inline float vec3Norm(float* _result, const float* _a)
	{
		const float len = vec3Length(_a);
		const float invLen = 1.0f/len;
		_result[0] = _a[0] * invLen;
		_result[1] = _a[1] * invLen;
		_result[2] = _a[2] * invLen;
		return len;
	}

	inline void vec3Min(float* _result, const float* _a, const float* _b)
	{
		_result[0] = fmin(_a[0], _b[0]);
		_result[1] = fmin(_a[1], _b[1]);
		_result[2] = fmin(_a[2], _b[2]);
	}

	inline void vec3Max(float* _result, const float* _a, const float* _b)
	{
		_result[0] = fmax(_a[0], _b[0]);
		_result[1] = fmax(_a[1], _b[1]);
		_result[2] = fmax(_a[2], _b[2]);
	}

	inline void vec3Rcp(float* _result, const float* _a)
	{
		_result[0] = 1.0f / _a[0];
		_result[1] = 1.0f / _a[1];
		_result[2] = 1.0f / _a[2];
	}

	inline void vec3TangentFrame(const float* _n, float* _t, float* _b)
	{
		const float nx = _n[0];
		const float ny = _n[1];
		const float nz = _n[2];

		if (bx::fabsolute(nx) > bx::fabsolute(nz) )
		{
			float invLen = 1.0f / bx::fsqrt(nx*nx + nz*nz);
			_t[0] = -nz * invLen;
			_t[1] =  0.0f;
			_t[2] =  nx * invLen;
		}
		else
		{
			float invLen = 1.0f / bx::fsqrt(ny*ny + nz*nz);
			_t[0] =  0.0f;
			_t[1] =  nz * invLen;
			_t[2] = -ny * invLen;
		}

		bx::vec3Cross(_b, _n, _t);
	}

	inline void vec3TangentFrame(const float* _n, float* _t, float* _b, float _angle)
	{
		vec3TangentFrame(_n, _t, _b);

		const float sa = fsin(_angle);
		const float ca = fcos(_angle);

		_t[0] = -sa * _b[0] + ca * _t[0];
		_t[1] = -sa * _b[1] + ca * _t[1];
		_t[2] = -sa * _b[2] + ca * _t[2];

		bx::vec3Cross(_b, _n, _t);
	}

	inline void quatIdentity(float* _result)
	{
		_result[0] = 0.0f;
		_result[1] = 0.0f;
		_result[2] = 0.0f;
		_result[3] = 1.0f;
	}

	inline void quatMove(float* _result, const float* _a)
	{
		_result[0] = _a[0];
		_result[1] = _a[1];
		_result[2] = _a[2];
		_result[3] = _a[3];
	}

	inline void quatMulXYZ(float* _result, const float* _qa, const float* _qb)
	{
		const float ax = _qa[0];
		const float ay = _qa[1];
		const float az = _qa[2];
		const float aw = _qa[3];

		const float bx = _qb[0];
		const float by = _qb[1];
		const float bz = _qb[2];
		const float bw = _qb[3];

		_result[0] = aw * bx + ax * bw + ay * bz - az * by;
		_result[1] = aw * by - ax * bz + ay * bw + az * bx;
		_result[2] = aw * bz + ax * by - ay * bx + az * bw;
	}

	inline void quatMul(float* _result, const float* _qa, const float* _qb)
	{
		const float ax = _qa[0];
		const float ay = _qa[1];
		const float az = _qa[2];
		const float aw = _qa[3];

		const float bx = _qb[0];
		const float by = _qb[1];
		const float bz = _qb[2];
		const float bw = _qb[3];

		_result[0] = aw * bx + ax * bw + ay * bz - az * by;
		_result[1] = aw * by - ax * bz + ay * bw + az * bx;
		_result[2] = aw * bz + ax * by - ay * bx + az * bw;
		_result[3] = aw * bw - ax * bx - ay * by - az * bz;
	}

	inline void quatInvert(float* _result, const float* _quat)
	{
		_result[0] = -_quat[0];
		_result[1] = -_quat[1];
		_result[2] = -_quat[2];
		_result[3] =  _quat[3];
	}

	inline float quatDot(const float* _a, const float* _b)
	{
		return _a[0]*_b[0]
			 + _a[1]*_b[1]
			 + _a[2]*_b[2]
			 + _a[3]*_b[3]
			 ;
	}

	inline void quatNorm(float* _result, const float* _quat)
	{
		const float norm = quatDot(_quat, _quat);
		if (0.0f < norm)
		{
			const float invNorm = 1.0f / fsqrt(norm);
			_result[0] = _quat[0] * invNorm;
			_result[1] = _quat[1] * invNorm;
			_result[2] = _quat[2] * invNorm;
			_result[3] = _quat[3] * invNorm;
		}
		else
		{
			quatIdentity(_result);
		}
	}

	inline void quatToEuler(float* _result, const float* _quat)
	{
		const float x = _quat[0];
		const float y = _quat[1];
		const float z = _quat[2];
		const float w = _quat[3];

		const float yy = y * y;
		const float zz = z * z;

		const float xx = x * x;
		_result[0] = fatan2(2.0f * (x * w - y * z), 1.0f - 2.0f * (xx + zz) );
		_result[1] = fatan2(2.0f * (y * w + x * z), 1.0f - 2.0f * (yy + zz) );
		_result[2] = fasin (2.0f * (x * y + z * w) );
	}

	inline void quatRotateAxis(float* _result, const float* _axis, float _angle)
	{
		const float ha = _angle * 0.5f;
		const float ca = fcos(ha);
		const float sa = fsin(ha);
		_result[0] = _axis[0] * sa;
		_result[1] = _axis[1] * sa;
		_result[2] = _axis[2] * sa;
		_result[3] = ca;
	}

	inline void quatRotateX(float* _result, float _ax)
	{
		const float hx = _ax * 0.5f;
		const float cx = fcos(hx);
		const float sx = fsin(hx);
		_result[0] = sx;
		_result[1] = 0.0f;
		_result[2] = 0.0f;
		_result[3] = cx;
	}

	inline void quatRotateY(float* _result, float _ay)
	{
		const float hy = _ay * 0.5f;
		const float cy = fcos(hy);
		const float sy = fsin(hy);
		_result[0] = 0.0f;
		_result[1] = sy;
		_result[2] = 0.0f;
		_result[3] = cy;
	}

	inline void quatRotateZ(float* _result, float _az)
	{
		const float hz = _az * 0.5f;
		const float cz = fcos(hz);
		const float sz = fsin(hz);
		_result[0] = 0.0f;
		_result[1] = 0.0f;
		_result[2] = sz;
		_result[3] = cz;
	}

	inline void vec3MulQuat(float* _result, const float* _vec, const float* _quat)
	{
		float tmp0[4];
		quatInvert(tmp0, _quat);

		float qv[4];
		qv[0] = _vec[0];
		qv[1] = _vec[1];
		qv[2] = _vec[2];
		qv[3] = 0.0f;

		float tmp1[4];
		quatMul(tmp1, tmp0, qv);

		quatMulXYZ(_result, tmp1, _quat);
	}

	inline void mtxIdentity(float* _result)
	{
		memSet(_result, 0, sizeof(float)*16);
		_result[0] = _result[5] = _result[10] = _result[15] = 1.0f;
	}

	inline void mtxTranslate(float* _result, float _tx, float _ty, float _tz)
	{
		mtxIdentity(_result);
		_result[12] = _tx;
		_result[13] = _ty;
		_result[14] = _tz;
	}

	inline void mtxScale(float* _result, float _sx, float _sy, float _sz)
	{
		memSet(_result, 0, sizeof(float) * 16);
		_result[0]  = _sx;
		_result[5]  = _sy;
		_result[10] = _sz;
		_result[15] = 1.0f;
	}

	inline void mtxScale(float* _result, float _scale)
	{
		mtxScale(_result, _scale, _scale, _scale);
	}

	inline void mtxFromNormal(float* _result, const float* _normal, float _scale, const float* _pos)
	{
		float tangent[3];
		float bitangent[3];
		vec3TangentFrame(_normal, tangent, bitangent);

		vec3Mul(&_result[ 0], bitangent, _scale);
		vec3Mul(&_result[ 4], _normal,   _scale);
		vec3Mul(&_result[ 8], tangent,   _scale);

		_result[ 3] = 0.0f;
		_result[ 7] = 0.0f;
		_result[11] = 0.0f;
		_result[12] = _pos[0];
		_result[13] = _pos[1];
		_result[14] = _pos[2];
		_result[15] = 1.0f;
	}

	inline void mtxFromNormal(float* _result, const float* _normal, float _scale, const float* _pos, float _angle)
	{
		float tangent[3];
		float bitangent[3];
		vec3TangentFrame(_normal, tangent, bitangent, _angle);

		vec3Mul(&_result[ 0], bitangent, _scale);
		vec3Mul(&_result[ 4], _normal,   _scale);
		vec3Mul(&_result[ 8], tangent,   _scale);

		_result[ 3] = 0.0f;
		_result[ 7] = 0.0f;
		_result[11] = 0.0f;
		_result[12] = _pos[0];
		_result[13] = _pos[1];
		_result[14] = _pos[2];
		_result[15] = 1.0f;
	}

	inline void mtxQuat(float* _result, const float* _quat)
	{
		const float x = _quat[0];
		const float y = _quat[1];
		const float z = _quat[2];
		const float w = _quat[3];

		const float x2  =  x + x;
		const float y2  =  y + y;
		const float z2  =  z + z;
		const float x2x = x2 * x;
		const float x2y = x2 * y;
		const float x2z = x2 * z;
		const float x2w = x2 * w;
		const float y2y = y2 * y;
		const float y2z = y2 * z;
		const float y2w = y2 * w;
		const float z2z = z2 * z;
		const float z2w = z2 * w;

		_result[ 0] = 1.0f - (y2y + z2z);
		_result[ 1] =         x2y - z2w;
		_result[ 2] =         x2z + y2w;
		_result[ 3] = 0.0f;

		_result[ 4] =         x2y + z2w;
		_result[ 5] = 1.0f - (x2x + z2z);
		_result[ 6] =         y2z - x2w;
		_result[ 7] = 0.0f;

		_result[ 8] =         x2z - y2w;
		_result[ 9] =         y2z + x2w;
		_result[10] = 1.0f - (x2x + y2y);
		_result[11] = 0.0f;

		_result[12] = 0.0f;
		_result[13] = 0.0f;
		_result[14] = 0.0f;
		_result[15] = 1.0f;
	}

	inline void mtxQuatTranslation(float* _result, const float* _quat, const float* _translation)
	{
		mtxQuat(_result, _quat);
		_result[12] = -(_result[0]*_translation[0] + _result[4]*_translation[1] + _result[ 8]*_translation[2]);
		_result[13] = -(_result[1]*_translation[0] + _result[5]*_translation[1] + _result[ 9]*_translation[2]);
		_result[14] = -(_result[2]*_translation[0] + _result[6]*_translation[1] + _result[10]*_translation[2]);
	}

	inline void mtxQuatTranslationHMD(float* _result, const float* _quat, const float* _translation)
	{
		float quat[4];
		quat[0] = -_quat[0];
		quat[1] = -_quat[1];
		quat[2] =  _quat[2];
		quat[3] =  _quat[3];
		mtxQuatTranslation(_result, quat, _translation);
	}

	inline void mtxLookAt_Impl(float* _result, const float* _eye, const float* _view, const float* _up)
	{
		float up[3] = { 0.0f, 1.0f, 0.0f };
		if (NULL != _up)
		{
			up[0] = _up[0];
			up[1] = _up[1];
			up[2] = _up[2];
		}

		float tmp[4];
		vec3Cross(tmp, up, _view);

		float right[4];
		vec3Norm(right, tmp);

		vec3Cross(up, _view, right);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = right[0];
		_result[ 1] = up[0];
		_result[ 2] = _view[0];

		_result[ 4] = right[1];
		_result[ 5] = up[1];
		_result[ 6] = _view[1];

		_result[ 8] = right[2];
		_result[ 9] = up[2];
		_result[10] = _view[2];

		_result[12] = -vec3Dot(right, _eye);
		_result[13] = -vec3Dot(up, _eye);
		_result[14] = -vec3Dot(_view, _eye);
		_result[15] = 1.0f;
	}

	inline void mtxLookAtLh(float* _result, const float* _eye, const float* _at, const float* _up)
	{
		float tmp[4];
		vec3Sub(tmp, _at, _eye);

		float view[4];
		vec3Norm(view, tmp);

		mtxLookAt_Impl(_result, _eye, view, _up);
	}

	inline void mtxLookAtRh(float* _result, const float* _eye, const float* _at, const float* _up)
	{
		float tmp[4];
		vec3Sub(tmp, _eye, _at);

		float view[4];
		vec3Norm(view, tmp);

		mtxLookAt_Impl(_result, _eye, view, _up);
	}

	inline void mtxLookAt(float* _result, const float* _eye, const float* _at, const float* _up)
	{
		mtxLookAtLh(_result, _eye, _at, _up);
	}

	template <Handness::Enum HandnessT>
	inline void mtxProjXYWH(float* _result, float _x, float _y, float _width, float _height, float _near, float _far, bool _oglNdc)
	{
		const float diff = _far-_near;
		const float aa = _oglNdc ?       (_far+_near)/diff : _far/diff;
		const float bb = _oglNdc ?  (2.0f*_far*_near)/diff : _near*aa;

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = _width;
		_result[ 5] = _height;
		_result[ 8] = (Handness::Right == HandnessT) ?    _x :  -_x;
		_result[ 9] = (Handness::Right == HandnessT) ?    _y :  -_y;
		_result[10] = (Handness::Right == HandnessT) ?   -aa :   aa;
		_result[11] = (Handness::Right == HandnessT) ? -1.0f : 1.0f;
		_result[14] = -bb;
	}

	template <Handness::Enum HandnessT>
	inline void mtxProj_impl(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc)
	{
		const float invDiffRl = 1.0f/(_rt - _lt);
		const float invDiffUd = 1.0f/(_ut - _dt);
		const float width  =  2.0f*_near * invDiffRl;
		const float height =  2.0f*_near * invDiffUd;
		const float xx     = (_rt + _lt) * invDiffRl;
		const float yy     = (_ut + _dt) * invDiffUd;
		mtxProjXYWH<HandnessT>(_result, xx, yy, width, height, _near, _far, _oglNdc);
	}

	template <Handness::Enum HandnessT>
	inline void mtxProj_impl(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<HandnessT>(_result, _fov[0], _fov[1], _fov[2], _fov[3], _near, _far, _oglNdc);
	}

	template <Handness::Enum HandnessT>
	inline void mtxProj_impl(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc)
	{
		const float height = 1.0f/ftan(toRad(_fovy)*0.5f);
		const float width  = height * 1.0f/_aspect;
		mtxProjXYWH<HandnessT>(_result, 0.0f, 0.0f, width, height, _near, _far, _oglNdc);
	}

	inline void mtxProj(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _ut, _dt, _lt, _rt, _near, _far, _oglNdc);
	}

	inline void mtxProj(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _fov, _near, _far, _oglNdc);
	}

	inline void mtxProj(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _fovy, _aspect, _near, _far, _oglNdc);
	}

	inline void mtxProjLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _ut, _dt, _lt, _rt, _near, _far, _oglNdc);
	}

	inline void mtxProjLh(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _fov, _near, _far, _oglNdc);
	}

	inline void mtxProjLh(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Left>(_result, _fovy, _aspect, _near, _far, _oglNdc);
	}

	inline void mtxProjRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Right>(_result, _ut, _dt, _lt, _rt, _near, _far, _oglNdc);
	}

	inline void mtxProjRh(float* _result, const float _fov[4], float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Right>(_result, _fov, _near, _far, _oglNdc);
	}

	inline void mtxProjRh(float* _result, float _fovy, float _aspect, float _near, float _far, bool _oglNdc)
	{
		mtxProj_impl<Handness::Right>(_result, _fovy, _aspect, _near, _far, _oglNdc);
	}

	template <NearFar::Enum NearFarT, Handness::Enum HandnessT>
	inline void mtxProjInfXYWH(float* _result, float _x, float _y, float _width, float _height, float _near, bool _oglNdc)
	{
		float aa;
		float bb;
		if (BX_ENABLED(NearFar::Reverse == NearFarT) )
		{
			aa = _oglNdc ?       -1.0f :   0.0f;
			bb = _oglNdc ? -2.0f*_near : -_near;
		}
		else
		{
			aa = 1.0f;
			bb = _oglNdc ? 2.0f*_near : _near;
		}

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = _width;
		_result[ 5] = _height;
		_result[ 8] = (Handness::Right == HandnessT) ?    _x :  -_x;
		_result[ 9] = (Handness::Right == HandnessT) ?    _y :  -_y;
		_result[10] = (Handness::Right == HandnessT) ?   -aa :   aa;
		_result[11] = (Handness::Right == HandnessT) ? -1.0f : 1.0f;
		_result[14] = -bb;
	}

	template <NearFar::Enum NearFarT, Handness::Enum HandnessT>
	inline void mtxProjInf_impl(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		const float invDiffRl = 1.0f/(_rt - _lt);
		const float invDiffUd = 1.0f/(_ut - _dt);
		const float width  =  2.0f*_near * invDiffRl;
		const float height =  2.0f*_near * invDiffUd;
		const float xx     = (_rt + _lt) * invDiffRl;
		const float yy     = (_ut + _dt) * invDiffUd;
		mtxProjInfXYWH<NearFarT,HandnessT>(_result, xx, yy, width, height, _near, _oglNdc);
	}

	template <NearFar::Enum NearFarT, Handness::Enum HandnessT>
	inline void mtxProjInf_impl(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFarT,HandnessT>(_result, _fov[0], _fov[1], _fov[2], _fov[3], _near, _oglNdc);
	}

	template <NearFar::Enum NearFarT, Handness::Enum HandnessT>
	inline void mtxProjInf_impl(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		const float height = 1.0f/ftan(toRad(_fovy)*0.5f);
		const float width  = height * 1.0f/_aspect;
		mtxProjInfXYWH<NearFarT,HandnessT>(_result, 0.0f, 0.0f, width, height, _near, _oglNdc);
	}

	inline void mtxProjInf(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _fov, _near, _oglNdc);
	}

	inline void mtxProjInf(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _ut, _dt, _lt, _rt, _near, _oglNdc);
	}

	inline void mtxProjInf(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _fovy, _aspect, _near, _oglNdc);
	}

	inline void mtxProjInfLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _ut, _dt, _lt, _rt, _near, _oglNdc);
	}

	inline void mtxProjInfLh(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _fov, _near, _oglNdc);
	}

	inline void mtxProjInfLh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Left>(_result, _fovy, _aspect, _near, _oglNdc);
	}

	inline void mtxProjInfRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Right>(_result, _ut, _dt, _lt, _rt, _near, _oglNdc);
	}

	inline void mtxProjInfRh(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Right>(_result, _fov, _near, _oglNdc);
	}

	inline void mtxProjInfRh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Default,Handness::Right>(_result, _fovy, _aspect, _near, _oglNdc);
	}

	inline void mtxProjRevInfLh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Left>(_result, _ut, _dt, _lt, _rt, _near, _oglNdc);
	}

	inline void mtxProjRevInfLh(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Left>(_result, _fov, _near, _oglNdc);
	}

	inline void mtxProjRevInfLh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Left>(_result, _fovy, _aspect, _near, _oglNdc);
	}

	inline void mtxProjRevInfRh(float* _result, float _ut, float _dt, float _lt, float _rt, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Right>(_result, _ut, _dt, _lt, _rt, _near, _oglNdc);
	}

	inline void mtxProjRevInfRh(float* _result, const float _fov[4], float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Right>(_result, _fov, _near, _oglNdc);
	}

	inline void mtxProjRevInfRh(float* _result, float _fovy, float _aspect, float _near, bool _oglNdc)
	{
		mtxProjInf_impl<NearFar::Reverse,Handness::Right>(_result, _fovy, _aspect, _near, _oglNdc);
	}

	template <Handness::Enum HandnessT>
	inline void mtxOrtho_impl(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc)
	{
		const float aa = 2.0f/(_right - _left);
		const float bb = 2.0f/(_top - _bottom);
		const float cc = (_oglNdc ? 2.0f : 1.0f) / (_far - _near);
		const float dd = (_left + _right)/(_left - _right);
		const float ee = (_top + _bottom)/(_bottom - _top);
		const float ff = _oglNdc ? (_near + _far)/(_near - _far) : _near/(_near - _far);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = aa;
		_result[ 5] = bb;
		_result[10] = (Handness::Right == HandnessT) ? -cc : cc;
		_result[12] = dd + _offset;
		_result[13] = ee;
		_result[14] = ff;
		_result[15] = 1.0f;
	}

	inline void mtxOrtho(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc)
	{
		mtxOrtho_impl<Handness::Left>(_result, _left, _right, _bottom, _top, _near, _far, _offset, _oglNdc);
	}

	inline void mtxOrthoLh(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc)
	{
		mtxOrtho_impl<Handness::Left>(_result, _left, _right, _bottom, _top, _near, _far, _offset, _oglNdc);
	}

	inline void mtxOrthoRh(float* _result, float _left, float _right, float _bottom, float _top, float _near, float _far, float _offset, bool _oglNdc)
	{
		mtxOrtho_impl<Handness::Right>(_result, _left, _right, _bottom, _top, _near, _far, _offset, _oglNdc);
	}

	inline void mtxRotateX(float* _result, float _ax)
	{
		const float sx = fsin(_ax);
		const float cx = fcos(_ax);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = 1.0f;
		_result[ 5] = cx;
		_result[ 6] = -sx;
		_result[ 9] = sx;
		_result[10] = cx;
		_result[15] = 1.0f;
	}

	inline void mtxRotateY(float* _result, float _ay)
	{
		const float sy = fsin(_ay);
		const float cy = fcos(_ay);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = cy;
		_result[ 2] = sy;
		_result[ 5] = 1.0f;
		_result[ 8] = -sy;
		_result[10] = cy;
		_result[15] = 1.0f;
	}

	inline void mtxRotateZ(float* _result, float _az)
	{
		const float sz = fsin(_az);
		const float cz = fcos(_az);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = cz;
		_result[ 1] = -sz;
		_result[ 4] = sz;
		_result[ 5] = cz;
		_result[10] = 1.0f;
		_result[15] = 1.0f;
	}

	inline void mtxRotateXY(float* _result, float _ax, float _ay)
	{
		const float sx = fsin(_ax);
		const float cx = fcos(_ax);
		const float sy = fsin(_ay);
		const float cy = fcos(_ay);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = cy;
		_result[ 2] = sy;
		_result[ 4] = sx*sy;
		_result[ 5] = cx;
		_result[ 6] = -sx*cy;
		_result[ 8] = -cx*sy;
		_result[ 9] = sx;
		_result[10] = cx*cy;
		_result[15] = 1.0f;
	}

	inline void mtxRotateXYZ(float* _result, float _ax, float _ay, float _az)
	{
		const float sx = fsin(_ax);
		const float cx = fcos(_ax);
		const float sy = fsin(_ay);
		const float cy = fcos(_ay);
		const float sz = fsin(_az);
		const float cz = fcos(_az);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = cy*cz;
		_result[ 1] = -cy*sz;
		_result[ 2] = sy;
		_result[ 4] = cz*sx*sy + cx*sz;
		_result[ 5] = cx*cz - sx*sy*sz;
		_result[ 6] = -cy*sx;
		_result[ 8] = -cx*cz*sy + sx*sz;
		_result[ 9] = cz*sx + cx*sy*sz;
		_result[10] = cx*cy;
		_result[15] = 1.0f;
	}

	inline void mtxRotateZYX(float* _result, float _ax, float _ay, float _az)
	{
		const float sx = fsin(_ax);
		const float cx = fcos(_ax);
		const float sy = fsin(_ay);
		const float cy = fcos(_ay);
		const float sz = fsin(_az);
		const float cz = fcos(_az);

		memSet(_result, 0, sizeof(float)*16);
		_result[ 0] = cy*cz;
		_result[ 1] = cz*sx*sy-cx*sz;
		_result[ 2] = cx*cz*sy+sx*sz;
		_result[ 4] = cy*sz;
		_result[ 5] = cx*cz + sx*sy*sz;
		_result[ 6] = -cz*sx + cx*sy*sz;
		_result[ 8] = -sy;
		_result[ 9] = cy*sx;
		_result[10] = cx*cy;
		_result[15] = 1.0f;
	};

	inline void mtxSRT(float* _result, float _sx, float _sy, float _sz, float _ax, float _ay, float _az, float _tx, float _ty, float _tz)
	{
		const float sx = fsin(_ax);
		const float cx = fcos(_ax);
		const float sy = fsin(_ay);
		const float cy = fcos(_ay);
		const float sz = fsin(_az);
		const float cz = fcos(_az);

		const float sxsz = sx*sz;
		const float cycz = cy*cz;

		_result[ 0] = _sx * (cycz - sxsz*sy);
		_result[ 1] = _sx * -cx*sz;
		_result[ 2] = _sx * (cz*sy + cy*sxsz);
		_result[ 3] = 0.0f;

		_result[ 4] = _sy * (cz*sx*sy + cy*sz);
		_result[ 5] = _sy * cx*cz;
		_result[ 6] = _sy * (sy*sz -cycz*sx);
		_result[ 7] = 0.0f;

		_result[ 8] = _sz * -cx*sy;
		_result[ 9] = _sz * sx;
		_result[10] = _sz * cx*cy;
		_result[11] = 0.0f;

		_result[12] = _tx;
		_result[13] = _ty;
		_result[14] = _tz;
		_result[15] = 1.0f;
	}

	inline void vec3MulMtx(float* _result, const float* _vec, const float* _mat)
	{
		_result[0] = _vec[0] * _mat[ 0] + _vec[1] * _mat[4] + _vec[2] * _mat[ 8] + _mat[12];
		_result[1] = _vec[0] * _mat[ 1] + _vec[1] * _mat[5] + _vec[2] * _mat[ 9] + _mat[13];
		_result[2] = _vec[0] * _mat[ 2] + _vec[1] * _mat[6] + _vec[2] * _mat[10] + _mat[14];
	}

	inline void vec3MulMtxH(float* _result, const float* _vec, const float* _mat)
	{
		float xx = _vec[0] * _mat[ 0] + _vec[1] * _mat[4] + _vec[2] * _mat[ 8] + _mat[12];
		float yy = _vec[0] * _mat[ 1] + _vec[1] * _mat[5] + _vec[2] * _mat[ 9] + _mat[13];
		float zz = _vec[0] * _mat[ 2] + _vec[1] * _mat[6] + _vec[2] * _mat[10] + _mat[14];
		float ww = _vec[0] * _mat[ 3] + _vec[1] * _mat[7] + _vec[2] * _mat[11] + _mat[15];
		float invW = fsign(ww)/ww;
		_result[0] = xx*invW;
		_result[1] = yy*invW;
		_result[2] = zz*invW;
	}

	inline void vec4MulMtx(float* _result, const float* _vec, const float* _mat)
	{
		_result[0] = _vec[0] * _mat[ 0] + _vec[1] * _mat[4] + _vec[2] * _mat[ 8] + _vec[3] * _mat[12];
		_result[1] = _vec[0] * _mat[ 1] + _vec[1] * _mat[5] + _vec[2] * _mat[ 9] + _vec[3] * _mat[13];
		_result[2] = _vec[0] * _mat[ 2] + _vec[1] * _mat[6] + _vec[2] * _mat[10] + _vec[3] * _mat[14];
		_result[3] = _vec[0] * _mat[ 3] + _vec[1] * _mat[7] + _vec[2] * _mat[11] + _vec[3] * _mat[15];
	}

	inline void mtxMul(float* _result, const float* _a, const float* _b)
	{
		vec4MulMtx(&_result[ 0], &_a[ 0], _b);
		vec4MulMtx(&_result[ 4], &_a[ 4], _b);
		vec4MulMtx(&_result[ 8], &_a[ 8], _b);
		vec4MulMtx(&_result[12], &_a[12], _b);
	}

	inline void mtxTranspose(float* _result, const float* _a)
	{
		_result[ 0] = _a[ 0];
		_result[ 4] = _a[ 1];
		_result[ 8] = _a[ 2];
		_result[12] = _a[ 3];
		_result[ 1] = _a[ 4];
		_result[ 5] = _a[ 5];
		_result[ 9] = _a[ 6];
		_result[13] = _a[ 7];
		_result[ 2] = _a[ 8];
		_result[ 6] = _a[ 9];
		_result[10] = _a[10];
		_result[14] = _a[11];
		_result[ 3] = _a[12];
		_result[ 7] = _a[13];
		_result[11] = _a[14];
		_result[15] = _a[15];
	}

	/// Convert LH to RH projection matrix and vice versa.
	inline void mtxProjFlipHandedness(float* _dst, const float* _src)
	{
		_dst[ 0] = -_src[ 0];
		_dst[ 1] = -_src[ 1];
		_dst[ 2] = -_src[ 2];
		_dst[ 3] = -_src[ 3];
		_dst[ 4] =  _src[ 4];
		_dst[ 5] =  _src[ 5];
		_dst[ 6] =  _src[ 6];
		_dst[ 7] =  _src[ 7];
		_dst[ 8] = -_src[ 8];
		_dst[ 9] = -_src[ 9];
		_dst[10] = -_src[10];
		_dst[11] = -_src[11];
		_dst[12] =  _src[12];
		_dst[13] =  _src[13];
		_dst[14] =  _src[14];
		_dst[15] =  _src[15];
	}

	/// Convert LH to RH view matrix and vice versa.
	inline void mtxViewFlipHandedness(float* _dst, const float* _src)
	{
		_dst[ 0] = -_src[ 0];
		_dst[ 1] =  _src[ 1];
		_dst[ 2] = -_src[ 2];
		_dst[ 3] =  _src[ 3];
		_dst[ 4] = -_src[ 4];
		_dst[ 5] =  _src[ 5];
		_dst[ 6] = -_src[ 6];
		_dst[ 7] =  _src[ 7];
		_dst[ 8] = -_src[ 8];
		_dst[ 9] =  _src[ 9];
		_dst[10] = -_src[10];
		_dst[11] =  _src[11];
		_dst[12] = -_src[12];
		_dst[13] =  _src[13];
		_dst[14] = -_src[14];
		_dst[15] =  _src[15];
	}

	inline void calcNormal(float _result[3], float _va[3], float _vb[3], float _vc[3])
	{
		float ba[3];
		vec3Sub(ba, _vb, _va);

		float ca[3];
		vec3Sub(ca, _vc, _va);

		float baxca[3];
		vec3Cross(baxca, ba, ca);

		vec3Norm(_result, baxca);
	}

	inline void calcPlane(float _result[4], float _va[3], float _vb[3], float _vc[3])
	{
		float normal[3];
		calcNormal(normal, _va, _vb, _vc);

		_result[0] = normal[0];
		_result[1] = normal[1];
		_result[2] = normal[2];
		_result[3] = -vec3Dot(normal, _va);
	}

	inline void calcLinearFit2D(float _result[2], const void* _points, uint32_t _stride, uint32_t _numPoints)
	{
		float sumX  = 0.0f;
		float sumY  = 0.0f;
		float sumXX = 0.0f;
		float sumXY = 0.0f;

		const uint8_t* ptr = (const uint8_t*)_points;
		for (uint32_t ii = 0; ii < _numPoints; ++ii, ptr += _stride)
		{
			const float* point = (const float*)ptr;
			float xx = point[0];
			float yy = point[1];
			sumX  += xx;
			sumY  += yy;
			sumXX += xx*xx;
			sumXY += xx*yy;
		}

		// [ sum(x^2) sum(x)    ] [ A ] = [ sum(x*y) ]
		// [ sum(x)   numPoints ] [ B ]   [ sum(y)   ]

		float det = (sumXX*_numPoints - sumX*sumX);
		float invDet = 1.0f/det;

		_result[0] = (-sumX * sumY + _numPoints * sumXY) * invDet;
		_result[1] = (sumXX * sumY - sumX       * sumXY) * invDet;
	}

	inline void calcLinearFit3D(float _result[3], const void* _points, uint32_t _stride, uint32_t _numPoints)
	{
		float sumX  = 0.0f;
		float sumY  = 0.0f;
		float sumZ  = 0.0f;
		float sumXX = 0.0f;
		float sumXY = 0.0f;
		float sumXZ = 0.0f;
		float sumYY = 0.0f;
		float sumYZ = 0.0f;

		const uint8_t* ptr = (const uint8_t*)_points;
		for (uint32_t ii = 0; ii < _numPoints; ++ii, ptr += _stride)
		{
			const float* point = (const float*)ptr;
			float xx = point[0];
			float yy = point[1];
			float zz = point[2];

			sumX  += xx;
			sumY  += yy;
			sumZ  += zz;
			sumXX += xx*xx;
			sumXY += xx*yy;
			sumXZ += xx*zz;
			sumYY += yy*yy;
			sumYZ += yy*zz;
		}

		// [ sum(x^2) sum(x*y) sum(x)    ] [ A ]   [ sum(x*z) ]
		// [ sum(x*y) sum(y^2) sum(y)    ] [ B ] = [ sum(y*z) ]
		// [ sum(x)   sum(y)   numPoints ] [ C ]   [ sum(z)   ]

		float mtx[9] =
		{
			sumXX, sumXY, sumX,
			sumXY, sumYY, sumY,
			sumX,  sumY,  float(_numPoints),
		};
		float invMtx[9];
		mtx3Inverse(invMtx, mtx);

		_result[0] = invMtx[0]*sumXZ + invMtx[1]*sumYZ + invMtx[2]*sumZ;
		_result[1] = invMtx[3]*sumXZ + invMtx[4]*sumYZ + invMtx[5]*sumZ;
		_result[2] = invMtx[6]*sumXZ + invMtx[7]*sumYZ + invMtx[8]*sumZ;
	}

	inline void rgbToHsv(float _hsv[3], const float _rgb[3])
	{
		const float rr = _rgb[0];
		const float gg = _rgb[1];
		const float bb = _rgb[2];

		const float s0 = fstep(bb, gg);

		const float px = flerp(bb,        gg,         s0);
		const float py = flerp(gg,        bb,         s0);
		const float pz = flerp(-1.0f,     0.0f,       s0);
		const float pw = flerp(2.0f/3.0f, -1.0f/3.0f, s0);

		const float s1 = fstep(px, rr);

		const float qx = flerp(px, rr, s1);
		const float qy = py;
		const float qz = flerp(pw, pz, s1);
		const float qw = flerp(rr, px, s1);

		const float dd = qx - fmin(qw, qy);
		const float ee = 1.0e-10f;

		_hsv[0] = fabsolute(qz + (qw - qy) / (6.0f * dd + ee) );
		_hsv[1] = dd / (qx + ee);
		_hsv[2] = qx;
	}

	inline void hsvToRgb(float _rgb[3], const float _hsv[3])
	{
		const float hh = _hsv[0];
		const float ss = _hsv[1];
		const float vv = _hsv[2];

		const float px = fabsolute(ffract(hh + 1.0f     ) * 6.0f - 3.0f);
		const float py = fabsolute(ffract(hh + 2.0f/3.0f) * 6.0f - 3.0f);
		const float pz = fabsolute(ffract(hh + 1.0f/3.0f) * 6.0f - 3.0f);

		_rgb[0] = vv * flerp(1.0f, fsaturate(px - 1.0f), ss);
		_rgb[1] = vv * flerp(1.0f, fsaturate(py - 1.0f), ss);
		_rgb[2] = vv * flerp(1.0f, fsaturate(pz - 1.0f), ss);
	}

} // namespace bx
