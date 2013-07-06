/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_FLOAT4_NEON_H__
#define __BX_FLOAT4_NEON_H__

#include <arm_neon.h>

namespace bx
{

// Reference:
// http://gcc.gnu.org/onlinedocs/gcc/ARM-NEON-Intrinsics.html
// http://blogs.arm.com/software-enablement/161-coding-for-neon-part-1-load-and-stores/
// http://blogs.arm.com/software-enablement/196-coding-for-neon-part-2-dealing-with-leftovers/
// http://blogs.arm.com/software-enablement/241-coding-for-neon-part-3-matrix-multiplication/
// http://blogs.arm.com/software-enablement/277-coding-for-neon-part-4-shifting-left-and-right/
// http://blogs.arm.com/software-enablement/684-coding-for-neon-part-5-rearranging-vectors/

	typedef __builtin_neon_sf float4_t __attribute__( (__vector_size__(16) ) );

#define ELEMx 0
#define ELEMy 1
#define ELEMz 2
#define ELEMw 3
#define IMPLEMENT_SWIZZLE(_x, _y, _z, _w) \
			BX_FLOAT4_INLINE float4_t float4_swiz_##_x##_y##_z##_w(float4_t _a) \
			{ \
				float4_t result; \
				result.ixyzw[0] = _a.ixyzw[ELEM##_x]; \
				result.ixyzw[1] = _a.ixyzw[ELEM##_y]; \
				result.ixyzw[2] = _a.ixyzw[ELEM##_z]; \
				result.ixyzw[3] = _a.ixyzw[ELEM##_w]; \
				return result; \
			}

#include "float4_swizzle.inl"

#undef IMPLEMENT_SWIZZLE
#undef ELEMw
#undef ELEMz
#undef ELEMy
#undef ELEMx

	BX_FLOAT4_INLINE float4_t float4_shuf_xyAB(float4_t _a, float4_t _b)
	{
		return _a; //_mm_movelh_ps(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_ABxy(float4_t _a, float4_t _b)
	{
		return _a; //_mm_movelh_ps(_b, _a);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_CDzw(float4_t _a, float4_t _b)
	{
		return _a; //_mm_movehl_ps(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_zwCD(float4_t _a, float4_t _b)
	{
		return _a; //_mm_movehl_ps(_b, _a);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_xAyB(float4_t _a, float4_t _b)
	{
		return _a; //_mm_unpacklo_ps(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_yBxA(float4_t _a, float4_t _b)
	{
		return _a; //_mm_unpacklo_ps(_b, _a);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_zCwD(float4_t _a, float4_t _b)
	{
		return _a; //_mm_unpackhi_ps(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_shuf_CzDw(float4_t _a, float4_t _b)
	{
		return _a; //_mm_unpackhi_ps(_b, _a);
	}

	BX_FLOAT4_INLINE float float4_x(float4_t _a)
	{
		return _a.fxyzw[0];
	}

	BX_FLOAT4_INLINE float float4_y(float4_t _a)
	{
		return _a.fxyzw[1];
	}

	BX_FLOAT4_INLINE float float4_z(float4_t _a)
	{
		return _a.fxyzw[2];
	}

	BX_FLOAT4_INLINE float float4_w(float4_t _a)
	{
		return _a.fxyzw[3];
	}

//	BX_FLOAT4_INLINE float4_t float4_ld(const void* _ptr)
//	{
//		return _mm_load_ps(reinterpret_cast<const float*>(_ptr) );
//	}

//	BX_FLOAT4_INLINE void float4_st(void* _ptr, float4_t _a)
//	{
//		_mm_store_ps(reinterpret_cast<float*>(_ptr), _a);
//	}

//	BX_FLOAT4_INLINE void float4_stream(void* _ptr, float4_t _a)
//	{
//		_mm_stream_ps(reinterpret_cast<float*>(_ptr), _a);
//	}

	BX_FLOAT4_INLINE float4_t float4_ld(float _x, float _y, float _z, float _w)
	{
		const float32_t val[4] = {_x, _y, _z, _w};
		return __builtin_neon_vld1v4sf(val);
	}

	BX_FLOAT4_INLINE float4_t float4_ild(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _w)
	{
		const uint32_t val[4] = {_x, _y, _z, _w};
		return (float4_t)__builtin_neon_vld1v4si( (const __builtin_neon_si*)val);
	}

	BX_FLOAT4_INLINE float4_t float4_splat(float _a)
	{
		return __builtin_neon_vdup_nv4sf(_a);
	}

	BX_FLOAT4_INLINE float4_t float4_isplat(uint32_t _a)
	{
		return (float4_t)__builtin_neon_vdup_nv4si( (__builtin_neon_si)_a);
	}

	BX_FLOAT4_INLINE float4_t float4_zero()
	{
		return vdupq_n_f32(0.0f);
	}

	BX_FLOAT4_INLINE float4_t float4_add(float4_t _a, float4_t _b)
	{
		return vaddq_f32(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_sub(float4_t _a, float4_t _b)
	{
		return vsubq_f32(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_mul(float4_t _a, float4_t _b)
	{
		return vmulq_f32(_a, _b);
	}

	BX_FLOAT4_INLINE float4_t float4_rcp_est(float4_t _a)
	{
		return vrecpeq_f32(_a);
	}

	BX_FLOAT4_INLINE float4_t float4_rsqrt_est(float4_t _a)
	{
		return vrsqrteq_f32(_a);
	}

	BX_FLOAT4_INLINE float4_t float4_and(float4_t _a, float4_t _b)
	{
		return (float4_t)__builtin_neon_vandv4si( (int32x4_t)_a, (int32x4_t)_b, 0);
	}

	//BX_FLOAT4_INLINE float4_t float4_andc(float4_t _a, float4_t _b)
	//{
	//	return _mm_andnot_ps(_b, _a);
	//}

	BX_FLOAT4_INLINE float4_t float4_or(float4_t _a, float4_t _b)
	{
		return (float4_t)__builtin_neon_vorrv4si( (int32x4_t)_a, (int32x4_t)_b, 0);
	}

	BX_FLOAT4_INLINE float4_t float4_iadd(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp0 = vreinterpretq_u32_f32(_a);
		const uint32x4_t tmp1 = vreinterpretq_u32_f32(_b);
		const uint32x4_t add  = vaddq_u32(tmp0, tmp1);
		const float4_t result = vreinterpretq_f32_u32(add);

		return result;
	}

	BX_FLOAT4_INLINE float4_t float4_isub(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp0 = vreinterpretq_u32_f32(_a);
		const uint32x4_t tmp1 = vreinterpretq_u32_f32(_b);
		const uint32x4_t sub  = vsubq_u32(tmp0, tmp1);
		const float4_t result = vreinterpretq_f32_u32(sub);

		return result;
	}

	BX_FLOAT4_INLINE float4_t float4_sll(float4_t _a, int _count)
	{
		const uint32x4_t tmp   = vreinterpretq_u32_f32(_a);
		const uint32x4_t shift = vshlq_n_u32(tmp, _count);
		const float4_t result  = vreinterpretq_f32_u32(shift);

		return result;
	}

	BX_FLOAT4_INLINE float4_t float4_srl(float4_t _a, int _count)
	{
		const uint32x4_t tmp   = vreinterpretq_i32_f32(_a);
		const uint32x4_t shift = (uint32x4_t)__builtin_neon_vshr_nv4si( (int32x4_t)tmp, _count, 0);
		const float4_t result  = vreinterpretq_f32_u32(shift);

		return result;
	}

	BX_FLOAT4_INLINE float4_t float4_sra(float4_t _a, int _count)
	{
		const int32x4_t a     = vreinterpretq_s32_f32(_a);
		const int32x4_t shift = __builtin_neon_vshr_nv4si(a, _count, 1);
		const float4_t result = vreinterpretq_f32_s32(shift);

		return result;
	}

} // namespace bx

#define float4_div_nr float4_div_nr_ni
#define float4_div float4_div_nr_ni
#define float4_ceil float4_ceil_ni
#define float4_floor float4_floor_ni
#include "float4_ni.h"

#endif // __BX_FLOAT4_NEON_H__
